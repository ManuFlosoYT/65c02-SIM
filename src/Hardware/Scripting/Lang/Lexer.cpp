#include "Lexer.h"
#include <charconv>
#include <unordered_map>

namespace System::Hardware::Scripting::Lang {

TokenType Lexer::getKeywordType(const std::string& text) {
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"let", TokenType::Let},
        {"if", TokenType::If},
        {"else", TokenType::Else},
        {"while", TokenType::While},
        {"break", TokenType::Break},
        {"continue", TokenType::Continue},
        {"true", TokenType::True},
        {"false", TokenType::False},
        {"print", TokenType::Print}
    };

    auto it = keywords.find(text);
    if (it != keywords.end()) {
        return it->second;
    }
    return TokenType::Identifier;
}

Lexer::Lexer(std::string source) : source(std::move(source)) {}

std::vector<Token> Lexer::scanTokens() {
    tokens.clear();
    start = 0;
    current = 0;
    line = 1;
    columnStart = 1;
    columnCurrent = 1;

    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) {
            break;
        }

        start = current;
        columnStart = columnCurrent;
        scanToken();
    }

    tokens.emplace_back(TokenType::Eof, "", line, columnCurrent);
    return tokens;
}

void Lexer::scanToken() {
    char c = advance();
    switch (c) {
        case '(': tokens.push_back(makeToken(TokenType::LeftParen)); break;
        case ')': tokens.push_back(makeToken(TokenType::RightParen)); break;
        case '{': tokens.push_back(makeToken(TokenType::LeftBrace)); break;
        case '}': tokens.push_back(makeToken(TokenType::RightBrace)); break;
        case '[': tokens.push_back(makeToken(TokenType::LeftBracket)); break;
        case ']': tokens.push_back(makeToken(TokenType::RightBracket)); break;
        case ',': tokens.push_back(makeToken(TokenType::Comma)); break;
        case '.': tokens.push_back(makeToken(TokenType::Dot)); break;
        case '-': tokens.push_back(makeToken(TokenType::Minus)); break;
        case '+': tokens.push_back(makeToken(TokenType::Plus)); break;
        case ';': tokens.push_back(makeToken(TokenType::Semicolon)); break;
        case '*': tokens.push_back(makeToken(TokenType::Star)); break;
        case '!':
            tokens.push_back(makeToken(match('=') ? TokenType::BangEqual : TokenType::Error));
            break;
        case '=':
            tokens.push_back(makeToken(match('=') ? TokenType::EqualEqual : TokenType::Equal));
            break;
        case '<':
            tokens.push_back(makeToken(match('=') ? TokenType::LessEqual : TokenType::Less));
            break;
        case '>':
            tokens.push_back(makeToken(match('=') ? TokenType::GreaterEqual : TokenType::Greater));
            break;
        case '/':
            if (match('/')) {
                // A comment goes until the end of the line.
                while (peek() != '\n' && !isAtEnd()) {
                    advance();
                }
            } else {
                tokens.push_back(makeToken(TokenType::Slash));
            }
            break;
        case '"': stringToken(); break;
        default:
            if (isDigit(c)) {
                numberToken();
            } else if (isAlpha(c)) {
                identifierToken();
            } else {
                tokens.push_back(errorToken("Unexpected character."));
            }
            break;
    }
}

void Lexer::skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                line++;
                advance();
                columnCurrent = 1; // Reset column after newline
                break;
            default:
                return;
        }
    }
}

char Lexer::advance() {
    current++;
    columnCurrent++;
    return source[current - 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd()) {
        return false;
    }
    if (source[current] != expected) {
        return false;
    }

    current++;
    columnCurrent++;
    return true;
}

char Lexer::peek() const {
    if (isAtEnd()) {
        return '\0';
    }
    return source[current];
}

char Lexer::peekNext() const {
    if (current + 1 >= source.length()) {
        return '\0';
    }
    return source[current + 1];
}

bool Lexer::isAtEnd() const {
    return current >= source.length();
}

void Lexer::stringToken() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            line++;
            columnCurrent = 1;
        }
        advance();
    }

    if (isAtEnd()) {
        tokens.push_back(errorToken("Unterminated string."));
        return;
    }

    // The closing ".
    advance();

    // Trim the surrounding quotes.
    std::string value = source.substr(start + 1, current - start - 2);
    tokens.push_back(makeToken(TokenType::String, value));
}

void Lexer::numberToken() {
    int base = 10;
    
    // Check for hex
    if (source[start] == '0' && (peek() == 'x' || peek() == 'X')) {
        advance(); // consume 'x'
        base = 16;
        while (isDigit(peek()) || (peek() >= 'a' && peek() <= 'f') || (peek() >= 'A' && peek() <= 'F')) {
            advance();
        }
    } else {
        while (isDigit(peek())) {
            advance();
        }
    }

    std::string numStr = source.substr(start, current - start);
    int64_t val = 0;
    
    if (base == 16) {
        auto result = std::from_chars(numStr.data() + 2, numStr.data() + numStr.size(), val, 16);
        if (result.ec != std::errc()) {
            tokens.push_back(errorToken("Invalid hex number format."));
            return;
        }
    } else {
        auto result = std::from_chars(numStr.data(), numStr.data() + numStr.size(), val, 10);
        if (result.ec != std::errc()) {
            tokens.push_back(errorToken("Invalid decimal number format."));
            return;
        }
    }

    tokens.emplace_back(TokenType::Number, numStr, line, columnStart, val);
}

void Lexer::identifierToken() {
    while (isAlphaNumeric(peek())) {
        advance();
    }

    std::string text = source.substr(start, current - start);
    tokens.emplace_back(makeToken(getKeywordType(text), text));
}

bool Lexer::isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

bool Lexer::isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

Token Lexer::makeToken(TokenType type) const {
    return {type, source.substr(start, current - start), line, columnStart};
}

Token Lexer::makeToken(TokenType type, const std::string& text) const {
    return {type, text, line, columnStart};
}

Token Lexer::errorToken(const std::string& message) const {
    return {TokenType::Error, message, line, columnStart};
}

} // namespace System::Hardware::Scripting::Lang
