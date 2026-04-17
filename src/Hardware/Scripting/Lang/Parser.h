#pragma once

#include "Token.h"
#include "AST.h"
#include <vector>
#include <stdexcept>
#include <span>

namespace System::Hardware::Scripting::Lang {

class ParseError : public std::runtime_error {
public:
    ParseError(Token token, const std::string& message) 
        : std::runtime_error(message), token(std::move(token)) {}
    Token token;
};

class Parser {
public:
    explicit Parser(std::span<const Token> tokens);
    std::vector<std::unique_ptr<Stmt>> parse();

private:
    std::unique_ptr<Stmt> declaration();
    std::unique_ptr<Stmt> varDeclaration();
    std::unique_ptr<Stmt> statement();
    std::unique_ptr<Stmt> printStatement();
    std::unique_ptr<Stmt> ifStatement();
    std::unique_ptr<Stmt> whileStatement();
    std::unique_ptr<Stmt> breakStatement();
    std::unique_ptr<Stmt> continueStatement();
    std::vector<std::unique_ptr<Stmt>> block();
    std::unique_ptr<Stmt> expressionStatement();

    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> assignment();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> call();
    std::unique_ptr<Expr> finishCall(std::unique_ptr<Expr> callee);
    std::unique_ptr<Expr> primary();

    bool match(std::initializer_list<TokenType> types);
    [[nodiscard]] bool check(TokenType type) const;
    Token advance();
    [[nodiscard]] bool isAtEnd() const;
    [[nodiscard]] Token peek() const;
    [[nodiscard]] Token previous() const;
    Token consume(TokenType type, const std::string& message);
    static ParseError error(const Token& token, const std::string& message);
    void synchronize();

    std::span<const Token> tokens;
    size_t current = 0;
};

} // namespace System::Hardware::Scripting::Lang
