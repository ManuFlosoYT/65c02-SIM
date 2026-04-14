#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <cstdint>
#include <variant>

namespace System::Hardware::Scripting::Lang {

enum class TokenType {
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
