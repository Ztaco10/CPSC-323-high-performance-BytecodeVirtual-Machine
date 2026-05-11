// scanner.h
// Defines the lexical scanner for the Lox language. The scanner
// reads a source string character by character and produces a
// sequence of tokens on demand. It uses maximal munch (longest
// match) and records the line number for error reporting.

#pragma once

#include <string>
#include <unordered_map>

// Enumeration of all possible token types in Lox. These include
// single‑character tokens, multi‑character tokens, literals, and
// keywords. See Chapters 14–15 of Crafting Interpreters for a
// similar definition.
enum class TokenType {
    // Single‑character tokens.
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS,
    SEMICOLON, SLASH, STAR,

    // One or two character tokens.
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,

    // Literals.
    IDENTIFIER, STRING, NUMBER,

    // Keywords.
    AND, CLASS, ELSE, FALSE,
    FUN, FOR, IF, NIL, OR,
    PRINT, RETURN, SUPER, THIS,
    TRUE, VAR, WHILE,

    END_OF_FILE
};

// A token produced by the scanner. It contains the token type,
// the substring of the source code that generated it, and the line
// number. We do not store the literal value here; literals are
// parsed in the compiler.
struct Token {
    TokenType type;
    std::string lexeme;
    int line;
};

// The Scanner class encapsulates all state needed to lex a
// Lox program. It maintains the source string and indices into it
// (`start` marks the beginning of the current lexeme, `current` is
// the character being examined). `line` tracks the current
// line number for error messages.
class Scanner {
public:
    explicit Scanner(const std::string &source);

    // Returns the next token from the input. When scanning
    // finishes, returns a token of type END_OF_FILE.
    Token scanToken();

private:
    const std::string &source;
    size_t start;
    size_t current;
    int line;

    // Helpers for scanning.
    bool isAtEnd() const;
    char advance();
    bool match(char expected);
    char peek() const;
    char peekNext() const;
    Token makeToken(TokenType type) const;
    Token identifier();
    Token number();
    Token string();

    // Check if a character is an alphabetic letter or underscore.
    static bool isAlpha(char c);
    // Check if a character is a digit 0–9.
    static bool isDigit(char c);

    // A map of reserved keywords to their token types. This is
    // static so it is initialized only once. See Section 14 of
    // Crafting Interpreters.
    static const std::unordered_map<std::string, TokenType> keywords;
};