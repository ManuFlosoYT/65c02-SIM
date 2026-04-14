#pragma once

#include "Token.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace System::Hardware::Scripting::Lang {

class Lexer {
public:
    Lexer(const std::string& source);

    std::vector<Token> scanTokens();

private:
    void scanToken();
    char advance();
    char peek() const;
    char peekNext() const;
    bool isAtEnd() const;
    bool match(char expected);

    void skipWhitespace();

    void stringToken();
    void numberToken();
    void identifierToken();

    bool isAlpha(char c) const;
    bool isDigit(char c) const;
    bool isAlphaNumeric(char c) const;

    Token makeToken(TokenType type);
    Token makeToken(TokenType type, const std::string& text);
    Token errorToken(const std::string& message);

    std::string source;
    size_t start = 0;
    size_t current = 0;
    int line = 1;
    int columnStart = 1;
    int columnCurrent = 1;

    std::vector<Token> tokens;

    static const std::unordered_map<std::string, TokenType> keywords;
};

} // namespace System::Hardware::Scripting::Lang
