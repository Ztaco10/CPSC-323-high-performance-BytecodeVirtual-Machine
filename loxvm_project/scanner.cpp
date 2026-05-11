// scanner.cpp
// Implements the lexical scanner defined in scanner.h. The
// scanner reads characters, skipping whitespace and comments,
// and produces tokens using maximal munch. Keywords are
// distinguished from identifiers via a lookup table.

#include "scanner.h"

#include <cctype>
#include <stdexcept>

// Initialize the static keyword map. These keywords correspond
// to reserved words in the Lox language. When scanning an
// identifier, we look it up here to see if it should be a
// keyword token instead.
const std::unordered_map<std::string, TokenType> Scanner::keywords = {
    {"and",    TokenType::AND},
    {"class",  TokenType::CLASS},
    {"else",   TokenType::ELSE},
    {"false",  TokenType::FALSE},
    {"for",    TokenType::FOR},
    {"fun",    TokenType::FUN},
    {"if",     TokenType::IF},
    {"nil",    TokenType::NIL},
    {"or",     TokenType::OR},
    {"print",  TokenType::PRINT},
    {"return", TokenType::RETURN},
    {"super",  TokenType::SUPER},
    {"this",   TokenType::THIS},
    {"true",   TokenType::TRUE},
    {"var",    TokenType::VAR},
    {"while",  TokenType::WHILE}
};

Scanner::Scanner(const std::string &src)
    : source(src), start(0), current(0), line(1) {}

bool Scanner::isAtEnd() const {
    return current >= source.size();
}

char Scanner::advance() {
    // Consume the current character and return it.
    return source[current++];
}

bool Scanner::match(char expected) {
    // If we've reached the end or the character doesn't match,
    // return false without consuming.
    if (isAtEnd()) return false;
    if (source[current] != expected) return false;
    current++;
    return true;
}

char Scanner::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Scanner::peekNext() const {
    if (current + 1 >= source.size()) return '\0';
    return source[current + 1];
}

static Token errorToken(const std::string &message, int line) {
    // When scanning fails, we return an error token. In this
    // implementation we embed the error message into the lexeme
    // field. The compiler will display it appropriately.
    return Token{TokenType::END_OF_FILE, message, line};
}

Token Scanner::makeToken(TokenType type) const {
    // Slice the source string from start to current to get the
    // lexeme. Note that std::string::substr copies the data.
    return Token{type, source.substr(start, current - start), line};
}

Token Scanner::scanToken() {
    // Skip whitespace and comments.
    while (true) {
        if (isAtEnd()) {
            return Token{TokenType::END_OF_FILE, "", line};
        }
        char c = peek();
        switch (c) {
        case ' ': case '\r': case '\t':
            // Ignore whitespace.
            advance();
            start = current;
            break;
        case '\n':
            // Track line numbers for error messages.
            line++;
            advance();
            start = current;
            break;
        case '/':
            if (peekNext() == '/') {
                // A comment goes until end of line.
                while (!isAtEnd() && peek() != '\n') advance();
            } else {
                // Not a comment; break out to handle '/' token.
                goto endSkip;
            }
            break;
        default:
            goto endSkip;
        }
    }
endSkip:
    start = current;
    if (isAtEnd()) {
        return Token{TokenType::END_OF_FILE, "", line};
    }
    char c = advance();
    // Single‑character tokens
    switch (c) {
    case '(': return makeToken(TokenType::LEFT_PAREN);
    case ')': return makeToken(TokenType::RIGHT_PAREN);
    case '{': return makeToken(TokenType::LEFT_BRACE);
    case '}': return makeToken(TokenType::RIGHT_BRACE);
    case ';': return makeToken(TokenType::SEMICOLON);
    case ',': return makeToken(TokenType::COMMA);
    case '.': return makeToken(TokenType::DOT);
    case '-': return makeToken(TokenType::MINUS);
    case '+': return makeToken(TokenType::PLUS);
    case '/': return makeToken(TokenType::SLASH);
    case '*': return makeToken(TokenType::STAR);

    // One or two character tokens
    case '!': return makeToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
    case '=': return makeToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
    case '<': return makeToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
    case '>': return makeToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);

    // Strings.
    case '"': return string();

    default:
        // Numbers and identifiers
        if (isDigit(c)) {
            return number();
        }
        if (isAlpha(c) || c == '_') {
            return identifier();
        }
        // Unexpected character.
        return errorToken("Unexpected character.", line);
    }
}

Token Scanner::string() {
    // Consume characters until we hit an unescaped closing quote
    // or the end of input. Lox does not support escape sequences,
    // so we simply scan until a matching '"' or newline.
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\n') line++;
        advance();
    }
    if (isAtEnd()) {
        return errorToken("Unterminated string.", line);
    }
    // The closing quote.
    advance();
    // Return the string token. We include the quotes in the lexeme;
    // the compiler will strip them when creating constants.
    return makeToken(TokenType::STRING);
}

Token Scanner::number() {
    // Consume the integer part.
    while (isDigit(peek())) advance();
    // Look for a fractional part.
    if (peek() == '.' && isDigit(peekNext())) {
        // Consume the '.'
        advance();
        // Consume the fractional digits.
        while (isDigit(peek())) advance();
    }
    return makeToken(TokenType::NUMBER);
}

Token Scanner::identifier() {
    while (isAlpha(peek()) || isDigit(peek()) || peek() == '_') {
        advance();
    }
    std::string text = source.substr(start, current - start);
    auto it = keywords.find(text);
    if (it != keywords.end()) {
        return Token{it->second, text, line};
    }
    return Token{TokenType::IDENTIFIER, text, line};
}

bool Scanner::isAlpha(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) != 0;
}

bool Scanner::isDigit(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}