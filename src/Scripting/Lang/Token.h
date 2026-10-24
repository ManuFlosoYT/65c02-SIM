#pragma once

#include <string>
#include <cstdint>

namespace System::Hardware::Scripting::Lang {

enum class TokenType : uint8_t {
    // Keywords
    Let,
    If,
    Else,
    While,
    Break,
    Continue,
    True,
    False,
    Print,
    Changed,

    // Types
    Identifier,
    Number, // Int64
    String,
    
    // Symbols
    Plus,           // +
    Minus,          // -
    Star,           // *
    Slash,          // /
    Equal,          // =
    EqualEqual,     // ==
    Bang,           // !
    BangEqual,      // !=
    Less,           // <
    LessEqual,      // <=
    Greater,        // >
    GreaterEqual,   // >=
    LeftParen,      // (
    RightParen,     // )
    LeftBrace,      // {
    RightBrace,     // }
    LeftBracket,    // [
    RightBracket,   // ]
    Comma,          // ,
    Dot,            // .
    Semicolon,      // ;
    AndAnd,         // &&
    OrOr,           // ||

    Eof,
    Error
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    int64_t numberValue = 0;

    Token(TokenType type, std::string lexeme, int line, int column)
        : type(type), lexeme(std::move(lexeme)), line(line), column(column) {}

    Token(TokenType type, std::string lexeme, int line, int column, int64_t numberValue)
        : type(type), lexeme(std::move(lexeme)), line(line), column(column), numberValue(numberValue) {}
};

} // namespace System::Hardware::Scripting::Lang
