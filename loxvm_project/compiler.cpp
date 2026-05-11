// compiler.cpp
// Implements the Lox compiler declared in compiler.h. This compiler
// uses recursive descent with a Pratt parser to emit bytecode as
// it parses. It avoids constructing an AST, instead emitting
// instructions directly into the Chunk passed in. For details
// on Pratt parsing see "Crafting Interpreters" Chapter 17.

#include "compiler.h"
#include "object.h"
#include "value.h"

#include <cctype>
#include <functional>
#include <iostream>

// Forward declarations of the compiler's internal parsing functions.
namespace {

    // Precedence levels for Pratt parsing. Larger values bind more
    // tightly. See compilePrecedence() below.
    enum class Precedence {
        NONE,
        ASSIGNMENT, // =
        OR,         // or
        AND,        // and
        EQUALITY,   // == !=
        COMPARISON, // > >= < <=
        TERM,       // + -
        FACTOR,     // * /
        UNARY,      // ! -
        CALL,       // () . (not used here)
        PRIMARY
    };

    struct Parser {
        Scanner scanner;
        Token current;
        Token previous;
        bool hadError = false;
        bool panicMode = false;

        Parser(const std::string &src) : scanner(src) {
            // Initialize current by advancing once.
            advance();
        }

        void advance() {
            previous = current;
            while (true) {
                current = scanner.scanToken();
                if (current.type != TokenType::END_OF_FILE) break;
                // END_OF_FILE used for errors as well; accept it.
                break;
            }
        }

        bool match(TokenType type) {
            if (current.type == type) {
                advance();
                return true;
            }
            return false;
        }

        void consume(TokenType type, const std::string &message) {
            if (current.type == type) {
                advance();
                return;
            }
            errorAtCurrent(message);
        }

        void errorAt(const Token &token, const std::string &message) {
            if (panicMode) return;
            panicMode = true;
            std::cerr << "[line " << token.line << "] Error";
            if (token.type == TokenType::END_OF_FILE) {
                std::cerr << " at end";
            } else {
                std::cerr << " at '" << token.lexeme << "'";
            }
            std::cerr << ": " << message << "\n";
            hadError = true;
        }

        void error(const std::string &message) {
            errorAt(previous, message);
        }

        void errorAtCurrent(const std::string &message) {
            errorAt(current, message);
        }

        void synchronize() {
            panicMode = false;
            while (current.type != TokenType::END_OF_FILE) {
                if (previous.type == TokenType::SEMICOLON) return;
                switch (current.type) {
                case TokenType::CLASS:
                case TokenType::FUN:
                case TokenType::VAR:
                case TokenType::FOR:
                case TokenType::IF:
                case TokenType::WHILE:
                case TokenType::PRINT:
                case TokenType::RETURN:
                    return;
                default:
                    break;
                }
                advance();
            }
        }
    };

    // Forward declaration of the compiler class so parse functions can refer to it.
    struct Compiler;

    using ParseFn = void (Compiler::*)(bool canAssign);
    struct ParseRuleStruct {
        ParseFn prefix;
        ParseFn infix;
        Precedence precedence;
    };

    // The main compiler class encapsulates state needed during compilation.
    struct Compiler {
        Parser parser;
        Chunk *chunk;
        VM &vm;

        Compiler(const std::string &source, VM &vm, Chunk *chunk)
            : parser(source), chunk(chunk), vm(vm) {}

        // Entry point: compile the entire program.
        bool compile() {
            while (parser.current.type != TokenType::END_OF_FILE) {
                declaration();
            }
            emitReturn();
            return !parser.hadError;
        }

        // === Bytecode emitting helpers ===
        void emitByte(uint8_t byte) {
            chunk->write(byte, parser.previous.line);
        }

        void emitBytes(uint8_t byte1, uint8_t byte2) {
            emitByte(byte1);
            emitByte(byte2);
        }

        void emitReturn() {
            emitByte(static_cast<uint8_t>(OpCode::OP_RETURN));
        }

        size_t makeConstant(Value value) {
            // Add value to constant pool. If the pool grows beyond 256
            // entries, we could use OP_CONSTANT_LONG with 3 bytes. For
            // simplicity we assume the program uses fewer constants.
            return chunk->addConstant(value);
        }

        void emitConstant(Value value) {
            size_t constant = makeConstant(value);
            emitBytes(static_cast<uint8_t>(OpCode::OP_CONSTANT), static_cast<uint8_t>(constant));
        }

        // Jump helpers. Emit an instruction followed by a placeholder
        // 16‑bit offset. Returns the offset of the operand so it can be
        // patched later.
        size_t emitJump(OpCode instruction) {
            emitByte(static_cast<uint8_t>(instruction));
            // Emit two placeholder bytes for the jump offset.
            emitByte(0xff);
            emitByte(0xff);
            return chunk->code.size() - 2;
        }

        void patchJump(size_t offset) {
            // Compute jump offset relative to the instruction just after
            // the jump operand.
            size_t jump = chunk->code.size() - offset - 2;
            if (jump > UINT16_MAX) {
                parser.error("Too much code to jump over.");
            }
            chunk->code[offset] = static_cast<uint8_t>((jump >> 8) & 0xff);
            chunk->code[offset + 1] = static_cast<uint8_t>(jump & 0xff);
        }

        void emitLoop(size_t loopStart) {
            emitByte(static_cast<uint8_t>(OpCode::OP_LOOP));
            size_t offset = chunk->code.size() - loopStart + 2;
            if (offset > UINT16_MAX) {
                parser.error("Loop body too large.");
            }
            emitByte(static_cast<uint8_t>((offset >> 8) & 0xff));
            emitByte(static_cast<uint8_t>(offset & 0xff));
        }

        // === Parsing ===
        void declaration() {
            if (parser.match(TokenType::VAR)) {
                varDeclaration();
            } else {
                statement();
            }
            if (parser.panicMode) parser.synchronize();
        }

        void varDeclaration() {
            // Expect variable name.
            parser.consume(TokenType::IDENTIFIER, "Expect variable name.");
            std::string name = parser.previous.lexeme;
            // Initialize variable if initializer present.
            Value initializer;
            bool hasInitializer = false;
            if (parser.match(TokenType::EQUAL)) {
                expression();
                hasInitializer = true;
            } else {
                emitByte(static_cast<uint8_t>(OpCode::OP_NIL));
            }
            parser.consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");
            // Add name to constant pool and define global.
            ObjString *nameObj = copyString(name, vm.strings, vm.objects);
            Value nameVal = makeObj(reinterpret_cast<Obj *>(nameObj));
            size_t global = makeConstant(nameVal);
            if (hasInitializer) {
                // value already on stack from initializer
            }
            emitBytes(static_cast<uint8_t>(OpCode::OP_DEFINE_GLOBAL), static_cast<uint8_t>(global));
        }

        void statement() {
            if (parser.match(TokenType::PRINT)) {
                printStatement();
            } else if (parser.match(TokenType::IF)) {
                ifStatement();
            } else if (parser.match(TokenType::WHILE)) {
                whileStatement();
            } else if (parser.match(TokenType::LEFT_BRACE)) {
                block();
            } else {
                expressionStatement();
            }
        }

        void printStatement() {
            expression();
            parser.consume(TokenType::SEMICOLON, "Expect ';' after value.");
            emitByte(static_cast<uint8_t>(OpCode::OP_PRINT));
        }

        void expressionStatement() {
            expression();
            parser.consume(TokenType::SEMICOLON, "Expect ';' after expression.");
            emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        }

        void ifStatement() {
            parser.consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
            expression();
            parser.consume(TokenType::RIGHT_PAREN, "Expect ')' after condition.");
            // Jump over then branch if false.
            size_t thenJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
            // Pop the condition.
            emitByte(static_cast<uint8_t>(OpCode::OP_POP));
            statement();
            // Jump over else branch.
            size_t elseJump = emitJump(OpCode::OP_JUMP);
            // Patch thenJump to here (start of else branch).
            patchJump(thenJump);
            // Pop condition for false branch.
            emitByte(static_cast<uint8_t>(OpCode::OP_POP));
            if (parser.match(TokenType::ELSE)) {
                statement();
            }
            // Patch elseJump to here (end of if).
            patchJump(elseJump);
        }

        void whileStatement() {
            // Remember the start of the loop.
            size_t loopStart = chunk->code.size();
            parser.consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'.");
            expression();
            parser.consume(TokenType::RIGHT_PAREN, "Expect ')' after condition.");
            // Jump out of the loop if condition is false.
            size_t exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
            // Pop the condition.
            emitByte(static_cast<uint8_t>(OpCode::OP_POP));
            statement();
            emitLoop(loopStart);
            // Patch exitJump to here.
            patchJump(exitJump);
            // Pop the condition for the false case.
            emitByte(static_cast<uint8_t>(OpCode::OP_POP));
        }

        void block() {
            while (parser.current.type != TokenType::RIGHT_BRACE &&
                   parser.current.type != TokenType::END_OF_FILE) {
                declaration();
            }
            parser.consume(TokenType::RIGHT_BRACE, "Expect '}' after block.");
        }

        // Top level expression parser. Allows assignment.
        void expression() {
            parsePrecedence(Precedence::ASSIGNMENT);
        }

        // Pratt parser entry point. Parses an expression at the given
        // precedence level. If canAssign is true, assignment is
        // allowed.
        void parsePrecedence(Precedence precedence) {
            parser.advance();
            ParseFn prefixRule = getRule(parser.previous.type).prefix;
            if (prefixRule == nullptr) {
                parser.error("Expect expression.");
                return;
            }
            bool canAssign = (static_cast<int>(precedence) <= static_cast<int>(Precedence::ASSIGNMENT));
            (this->*prefixRule)(canAssign);
            while (static_cast<int>(precedence) <= static_cast<int>(getRule(parser.current.type).precedence)) {
                parser.advance();
                ParseFn infixRule = getRule(parser.previous.type).infix;
                (this->*infixRule)(canAssign);
            }
            if (canAssign && parser.match(TokenType::EQUAL)) {
                parser.error("Invalid assignment target.");
                expression();
            }
        }

        // === Parse functions ===
        // Handle grouping: ( expression )
        void grouping(bool canAssign) {
            (void)canAssign;
            expression();
            parser.consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
        }

        void number(bool canAssign) {
            (void)canAssign;
            double value = std::stod(parser.previous.lexeme);
            emitConstant(makeNumber(value));
        }

        void string_(bool canAssign) {
            (void)canAssign;
            // Strip the surrounding quotes.
            std::string raw = parser.previous.lexeme;
            if (raw.size() >= 2) {
                raw = raw.substr(1, raw.size() - 2);
            }
            ObjString *obj = copyString(raw, vm.strings, vm.objects);
            Value val = makeObj(reinterpret_cast<Obj *>(obj));
            emitConstant(val);
        }

        void literal(bool canAssign) {
            (void)canAssign;
            switch (parser.previous.type) {
            case TokenType::FALSE: emitByte(static_cast<uint8_t>(OpCode::OP_FALSE)); break;
            case TokenType::TRUE:  emitByte(static_cast<uint8_t>(OpCode::OP_TRUE)); break;
            case TokenType::NIL:   emitByte(static_cast<uint8_t>(OpCode::OP_NIL)); break;
            default: return; // Unreachable
            }
        }

        void variable(bool canAssign) {
            namedVariable(parser.previous, canAssign);
        }

        void namedVariable(const Token &name, bool canAssign) {
            // Load the name as a string constant. Names are stored
            // without quotes.
            ObjString *obj = copyString(name.lexeme, vm.strings, vm.objects);
            Value val = makeObj(reinterpret_cast<Obj *>(obj));
            size_t arg = makeConstant(val);
            if (canAssign && parser.match(TokenType::EQUAL)) {
                // Assignment to this variable.
                expression();
                emitBytes(static_cast<uint8_t>(OpCode::OP_SET_GLOBAL), static_cast<uint8_t>(arg));
            } else {
                emitBytes(static_cast<uint8_t>(OpCode::OP_GET_GLOBAL), static_cast<uint8_t>(arg));
            }
        }

        void unary(bool canAssign) {
            (void)canAssign;
            TokenType operatorType = parser.previous.type;
            // Compile the operand.
            parsePrecedence(Precedence::UNARY);
            // Emit the operator instruction.
            switch (operatorType) {
            case TokenType::BANG: emitByte(static_cast<uint8_t>(OpCode::OP_NOT)); break;
            case TokenType::MINUS: emitByte(static_cast<uint8_t>(OpCode::OP_NEGATE)); break;
            default: return; // Unreachable.
            }
        }

        void binary(bool canAssign) {
            (void)canAssign;
            TokenType operatorType = parser.previous.type;
            // Compile the right operand. For left associativity, the
            // precedence of the operator is one level higher than
            // the current precedence.
            Precedence rulePrecedence = getRule(operatorType).precedence;
            parsePrecedence(static_cast<Precedence>(static_cast<int>(rulePrecedence) + 1));
            switch (operatorType) {
            case TokenType::PLUS:      emitByte(static_cast<uint8_t>(OpCode::OP_ADD)); break;
            case TokenType::MINUS:     emitByte(static_cast<uint8_t>(OpCode::OP_SUBTRACT)); break;
            case TokenType::STAR:      emitByte(static_cast<uint8_t>(OpCode::OP_MULTIPLY)); break;
            case TokenType::SLASH:     emitByte(static_cast<uint8_t>(OpCode::OP_DIVIDE)); break;
            case TokenType::BANG_EQUAL: emitByte(static_cast<uint8_t>(OpCode::OP_EQUAL)); emitByte(static_cast<uint8_t>(OpCode::OP_NOT)); break;
            case TokenType::EQUAL_EQUAL: emitByte(static_cast<uint8_t>(OpCode::OP_EQUAL)); break;
            case TokenType::GREATER:      emitByte(static_cast<uint8_t>(OpCode::OP_GREATER)); break;
            case TokenType::GREATER_EQUAL: emitByte(static_cast<uint8_t>(OpCode::OP_LESS)); emitByte(static_cast<uint8_t>(OpCode::OP_NOT)); break;
            case TokenType::LESS:          emitByte(static_cast<uint8_t>(OpCode::OP_LESS)); break;
            case TokenType::LESS_EQUAL:    emitByte(static_cast<uint8_t>(OpCode::OP_GREATER)); emitByte(static_cast<uint8_t>(OpCode::OP_NOT)); break;
            default: return; // Unreachable.
            }
        }

        void and_(bool canAssign) {
            (void)canAssign;
            // Short circuit on false. If the left operand is false,
            // jump over the right operand and leave false on the stack.
            size_t endJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
            // Pop the left operand before evaluating the right.
            emitByte(static_cast<uint8_t>(OpCode::OP_POP));
            parsePrecedence(Precedence::AND);
            patchJump(endJump);
        }

        void or_(bool canAssign) {
            (void)canAssign;
            // If left is true, skip evaluating right. Jump to second
            // jump leaving the true on stack.
            size_t elseJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
            size_t endJump = emitJump(OpCode::OP_JUMP);
            patchJump(elseJump);
            // Pop falsey left value before evaluating right.
            emitByte(static_cast<uint8_t>(OpCode::OP_POP));
            parsePrecedence(Precedence::OR);
            patchJump(endJump);
        }

        // Placeholder for call operator (not implemented). Using this
        // function prevents nullptr function pointers in parse rules.
        void callNothing(bool canAssign) {
            (void)canAssign;
            parser.error("Unexpected token.");
        }

        // Look up the parse rule for a given token type.
        ParseRuleStruct getRule(TokenType type) {
            // Static table of parse rules. For types with no prefix or
            // infix function, we store nullptr.
            static const ParseRuleStruct rules[] = {
                /* LEFT_PAREN    */ {&Compiler::grouping,    &Compiler::callNothing,    Precedence::CALL},
                /* RIGHT_PAREN   */ {nullptr,                nullptr,                  Precedence::NONE},
                /* LEFT_BRACE    */ {nullptr,                nullptr,                  Precedence::NONE},
                /* RIGHT_BRACE   */ {nullptr,                nullptr,                  Precedence::NONE},
                /* COMMA         */ {nullptr,                nullptr,                  Precedence::NONE},
                /* DOT           */ {nullptr,                nullptr,                  Precedence::CALL},
                /* MINUS         */ {&Compiler::unary,       &Compiler::binary,        Precedence::TERM},
                /* PLUS          */ {nullptr,                &Compiler::binary,        Precedence::TERM},
                /* SEMICOLON     */ {nullptr,                nullptr,                  Precedence::NONE},
                /* SLASH         */ {nullptr,                &Compiler::binary,        Precedence::FACTOR},
                /* STAR          */ {nullptr,                &Compiler::binary,        Precedence::FACTOR},
                /* BANG          */ {&Compiler::unary,       nullptr,                  Precedence::NONE},
                /* BANG_EQUAL    */ {nullptr,                &Compiler::binary,        Precedence::EQUALITY},
                /* EQUAL         */ {nullptr,                nullptr,                  Precedence::NONE},
                /* EQUAL_EQUAL   */ {nullptr,                &Compiler::binary,        Precedence::EQUALITY},
                /* GREATER       */ {nullptr,                &Compiler::binary,        Precedence::COMPARISON},
                /* GREATER_EQUAL */ {nullptr,                &Compiler::binary,        Precedence::COMPARISON},
                /* LESS          */ {nullptr,                &Compiler::binary,        Precedence::COMPARISON},
                /* LESS_EQUAL    */ {nullptr,                &Compiler::binary,        Precedence::COMPARISON},
                /* IDENTIFIER    */ {&Compiler::variable,    nullptr,                  Precedence::NONE},
                /* STRING        */ {&Compiler::string_,     nullptr,                  Precedence::NONE},
                /* NUMBER        */ {&Compiler::number,      nullptr,                  Precedence::NONE},
                /* AND           */ {nullptr,                &Compiler::and_,          Precedence::AND},
                /* CLASS         */ {nullptr,                nullptr,                  Precedence::NONE},
                /* ELSE          */ {nullptr,                nullptr,                  Precedence::NONE},
                /* FALSE         */ {&Compiler::literal,     nullptr,                  Precedence::NONE},
                /* FUN           */ {nullptr,                nullptr,                  Precedence::NONE},
                /* FOR           */ {nullptr,                nullptr,                  Precedence::NONE},
                /* IF            */ {nullptr,                nullptr,                  Precedence::NONE},
                /* NIL           */ {&Compiler::literal,     nullptr,                  Precedence::NONE},
                /* OR            */ {nullptr,                &Compiler::or_,           Precedence::OR},
                /* PRINT         */ {nullptr,                nullptr,                  Precedence::NONE},
                /* RETURN        */ {nullptr,                nullptr,                  Precedence::NONE},
                /* SUPER         */ {nullptr,                nullptr,                  Precedence::NONE},
                /* THIS          */ {nullptr,                nullptr,                  Precedence::NONE},
                /* TRUE          */ {&Compiler::literal,     nullptr,                  Precedence::NONE},
                /* VAR           */ {nullptr,                nullptr,                  Precedence::NONE},
                /* WHILE         */ {nullptr,                nullptr,                  Precedence::NONE},
                /* END_OF_FILE   */ {nullptr,                nullptr,                  Precedence::NONE}
            };
            // TokenType values start at 0 and are contiguous; cast to size_t.
            return rules[static_cast<size_t>(type)];
        }

        
    };

} // end anonymous namespace

// Compile the source code into bytecode and return the resulting
// chunk. On error returns nullptr.
Chunk *compile(const std::string &source, VM &vm) {
    Chunk *chunk = new Chunk();
    Compiler compiler(source, vm, chunk);
    if (!compiler.compile()) {
        delete chunk;
        return nullptr;
    }
    return chunk;
}