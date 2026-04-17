#pragma once

#include "Token.h"
#include <string>
#include <vector>

namespace System::Hardware::Scripting::Lang {

class Lexer {
public:
    explicit Lexer(std::string source);

    std::vector<Token> scanTokens();

private:
    void scanToken();
    char advance();
    [[nodiscard]] char peek() const;
    [[nodiscard]] char peekNext() const;
    [[nodiscard]] bool isAtEnd() const;
    bool match(char expected);

    void skipWhitespace();

    void stringToken();
    void numberToken();
    void identifierToken();

    static bool isAlpha(char c);
    static bool isDigit(char c);
    static bool isAlphaNumeric(char c);

    [[nodiscard]] Token makeToken(TokenType type) const;
    [[nodiscard]] Token makeToken(TokenType type, const std::string& text) const;
    [[nodiscard]] Token errorToken(const std::string& message) const;

    std::string source;
    size_t start = 0;
    size_t current = 0;
    int line = 1;
    int columnStart = 1;
    int columnCurrent = 1;

    std::vector<Token> tokens;

    static TokenType getKeywordType(const std::string& text);
};

} // namespace System::Hardware::Scripting::Lang
