// vm.cpp
// Implements the VM defined in vm.h. The VM executes bytecode
// produced by the compiler using a simple stack machine. It
// supports global variables, arithmetic, comparisons, control
// flow, and string interning.

#include "vm.h"
#include "object.h"

#include <iostream>

VM::VM() : ip(0), currentChunk(nullptr) {
    stack.reserve(256);
}

VM::~VM() {
    freeVM();
}

void VM::freeVM() {
    // Free all heap‑allocated objects.
    for (Obj *obj : objects) {
        // Determine object type and delete accordingly.
        if (obj->type == ObjType::STRING) {
            delete reinterpret_cast<ObjString *>(obj);
        } else {
            // If additional types are added, handle them here.
            delete obj;
        }
    }
    objects.clear();
    strings.clear();
    globals.clear();
    resetStack();
}

void VM::resetStack() {
    stack.clear();
}

void VM::push(const Value &value) {
    stack.push_back(value);
}

Value VM::pop() {
    Value v = stack.back();
    stack.pop_back();
    return v;
}

Value VM::peek(int distance) const {
    return stack[stack.size() - 1 - distance];
}

InterpretResult VM::runtimeError(const std::string &message) {
    std::cerr << message << "\n";
    // Show the line number where the error occurred.
    size_t instruction = ip - 1;
    int line = currentChunk->lines[instruction];
    std::cerr << "[line " << line << "] in script\n";
    resetStack();
    return InterpretResult::RUNTIME_ERROR;
}

InterpretResult VM::interpret(Chunk *chunk) {
    currentChunk = chunk;
    ip = 0;
    while (ip < currentChunk->code.size()) {
        if (!runInstruction()) {
            break;
        }
    }
    return InterpretResult::OK;
}

bool VM::runInstruction() {
    uint8_t instruction = currentChunk->code[ip++];
    switch (static_cast<OpCode>(instruction)) {
    case OpCode::OP_CONSTANT: {
        uint8_t constantIndex = currentChunk->code[ip++];
        push(currentChunk->constants[constantIndex]);
        break;
    }
    case OpCode::OP_NIL:
        push(makeNil());
        break;
    case OpCode::OP_TRUE:
        push(makeBool(true));
        break;
    case OpCode::OP_FALSE:
        push(makeBool(false));
        break;
    case OpCode::OP_POP:
        pop();
        break;
    case OpCode::OP_GET_GLOBAL: {
        uint8_t constantIndex = currentChunk->code[ip++];
        Value nameVal = currentChunk->constants[constantIndex];
        ObjString *nameObj = asString(nameVal);
        auto it = globals.find(nameObj->chars);
        if (it == globals.end()) {
            runtimeError("Undefined variable '" + nameObj->chars + "'.");
            return false;
        }
        push(it->second);
        break;
    }
    case OpCode::OP_DEFINE_GLOBAL: {
        uint8_t constantIndex = currentChunk->code[ip++];
        Value nameVal = currentChunk->constants[constantIndex];
        ObjString *nameObj = asString(nameVal);
        Value value = pop();
        globals[nameObj->chars] = value;
        break;
    }
    case OpCode::OP_SET_GLOBAL: {
        uint8_t constantIndex = currentChunk->code[ip++];
        Value nameVal = currentChunk->constants[constantIndex];
        ObjString *nameObj = asString(nameVal);
        if (globals.find(nameObj->chars) == globals.end()) {
            runtimeError("Undefined variable '" + nameObj->chars + "'.");
            return false;
        }
        // Assign to global and leave value on the stack.
        globals[nameObj->chars] = peek(0);
        break;
    }
    case OpCode::OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(makeBool(valuesEqual(a, b)));
        break;
    }
    case OpCode::OP_GREATER: {
        Value b = pop();
        Value a = pop();
        if (!isNumber(a) || !isNumber(b)) {
            runtimeError("Operands must be numbers.");
            return false;
        }
        push(makeBool(asNumber(a) > asNumber(b)));
        break;
    }
    case OpCode::OP_LESS: {
        Value b = pop();
        Value a = pop();
        if (!isNumber(a) || !isNumber(b)) {
            runtimeError("Operands must be numbers.");
            return false;
        }
        push(makeBool(asNumber(a) < asNumber(b)));
        break;
    }
    case OpCode::OP_ADD: {
        Value b = pop();
        Value a = pop();
        // If both operands are strings, concatenate.
        if (isObj(a) && asObj(a)->type == ObjType::STRING && isObj(b) && asObj(b)->type == ObjType::STRING) {
            ObjString *astr = asString(a);
            ObjString *bstr = asString(b);
            std::string combined = astr->chars + bstr->chars;
            ObjString *concat = copyString(combined, strings, objects);
            push(makeObj(reinterpret_cast<Obj *>(concat)));
        } else if (isNumber(a) && isNumber(b)) {
            push(makeNumber(asNumber(a) + asNumber(b)));
        } else {
            runtimeError("Operands must be two numbers or two strings.");
            return false;
        }
        break;
    }
    case OpCode::OP_SUBTRACT: {
        Value b = pop();
        Value a = pop();
        if (!isNumber(a) || !isNumber(b)) {
            runtimeError("Operands must be numbers.");
            return false;
        }
        push(makeNumber(asNumber(a) - asNumber(b)));
        break;
    }
    case OpCode::OP_MULTIPLY: {
        Value b = pop();
        Value a = pop();
        if (!isNumber(a) || !isNumber(b)) {
            runtimeError("Operands must be numbers.");
            return false;
        }
        push(makeNumber(asNumber(a) * asNumber(b)));
        break;
    }
    case OpCode::OP_DIVIDE: {
        Value b = pop();
        Value a = pop();
        if (!isNumber(a) || !isNumber(b)) {
            runtimeError("Operands must be numbers.");
            return false;
        }
        if (asNumber(b) == 0) {
            runtimeError("Division by zero.");
            return false;
        }
        push(makeNumber(asNumber(a) / asNumber(b)));
        break;
    }
    case OpCode::OP_NOT: {
        Value value = pop();
        bool result;
        if (isNil(value)) result = true;
        else if (isBool(value)) result = !asBool(value);
        else result = false;
        push(makeBool(result));
        break;
    }
    case OpCode::OP_NEGATE: {
        Value value = pop();
        if (!isNumber(value)) {
            runtimeError("Operand must be a number.");
            return false;
        }
        push(makeNumber(-asNumber(value)));
        break;
    }
    case OpCode::OP_PRINT: {
        Value value = pop();
        printValue(value);
        std::cout << "\n";
        break;
    }
    case OpCode::OP_JUMP: {
        uint16_t offset = static_cast<uint16_t>(currentChunk->code[ip++] << 8);
        offset |= currentChunk->code[ip++];
        ip += offset;
        break;
    }
    case OpCode::OP_JUMP_IF_FALSE: {
        uint16_t offset = static_cast<uint16_t>(currentChunk->code[ip++] << 8);
        offset |= currentChunk->code[ip++];
        Value value = peek(0);
        bool isFalsey;
        if (isNil(value)) isFalsey = true;
        else if (isBool(value)) isFalsey = !asBool(value);
        else isFalsey = false;
        if (isFalsey) {
            ip += offset;
        }
        break;
    }
    case OpCode::OP_LOOP: {
        uint16_t offset = static_cast<uint16_t>(currentChunk->code[ip++] << 8);
        offset |= currentChunk->code[ip++];
        ip -= offset;
        break;
    }
    case OpCode::OP_RETURN:
        // Terminate execution.
        return false;
    default:
        runtimeError("Unknown opcode.");
        return false;
    }
    return true;
}

void VM::concatenate() {
    // Pop the second operand then the first. They must both be strings.
    Value bVal = pop();
    Value aVal = pop();
    ObjString *b = asString(bVal);
    ObjString *a = asString(aVal);
    std::string result = a->chars + b->chars;
    ObjString *concat = copyString(result, strings, objects);
    push(makeObj(reinterpret_cast<Obj *>(concat)));
}