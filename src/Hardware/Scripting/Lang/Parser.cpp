#include "Parser.h"
#include <algorithm>

namespace System::Hardware::Scripting::Lang {

Parser::Parser(std::span<const Token> tokens) : tokens(tokens) {}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!isAtEnd()) {
        auto decl = declaration();
        if (decl) {
            statements.push_back(std::move(decl));
        }
    }
    return statements;
}

std::unique_ptr<Stmt> Parser::declaration() {
    try {
        if (match({TokenType::Let})) {
            return varDeclaration();
        }

        return statement();
    } catch (ParseError& error) {
        synchronize();
        return nullptr;
    }
}

std::unique_ptr<Stmt> Parser::varDeclaration() {
    Token name = consume(TokenType::Identifier, "Expect variable name.");

    std::unique_ptr<Expr> initializer = nullptr;
    if (match({TokenType::Equal})) {
        initializer = expression();
    }

    consume(TokenType::Semicolon, "Expect ';' after variable declaration.");
    return std::make_unique<VarStmt>(name, std::move(initializer));
}

std::unique_ptr<Stmt> Parser::statement() {
    if (match({TokenType::If})) {
        return ifStatement();
    }
    if (match({TokenType::While})) {
        return whileStatement();
    }
    if (match({TokenType::Break})) {
        return breakStatement();
    }
    if (match({TokenType::Continue})) {
        return continueStatement();
    }
    if (match({TokenType::Print})) {
        return printStatement();
    }
    if (match({TokenType::LeftBrace})) {
        return std::make_unique<BlockStmt>(block());
    }

    return expressionStatement();
}

std::unique_ptr<Stmt> Parser::printStatement() {
    std::unique_ptr<Expr> value = expression();
    consume(TokenType::Semicolon, "Expect ';' after value.");
    return std::make_unique<PrintStmt>(std::move(value));
}

std::unique_ptr<Stmt> Parser::ifStatement() {
    consume(TokenType::LeftParen, "Expect '(' after 'if'.");
    std::unique_ptr<Expr> condition = expression();
    consume(TokenType::RightParen, "Expect ')' after if condition.");

    std::unique_ptr<Stmt> thenBranch = statement();
    std::unique_ptr<Stmt> elseBranch = nullptr;
    if (match({TokenType::Else})) {
        elseBranch = statement();
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<Stmt> Parser::whileStatement() {
    consume(TokenType::LeftParen, "Expect '(' after 'while'.");
    std::unique_ptr<Expr> condition = expression();
    consume(TokenType::RightParen, "Expect ')' after condition.");
    std::unique_ptr<Stmt> body = statement();

    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Stmt> Parser::breakStatement() {
    Token keyword = previous();
    consume(TokenType::Semicolon, "Expect ';' after 'break'.");
    return std::make_unique<BreakStmt>(keyword);
}

std::unique_ptr<Stmt> Parser::continueStatement() {
    Token keyword = previous();
    consume(TokenType::Semicolon, "Expect ';' after 'continue'.");
    return std::make_unique<ContinueStmt>(keyword);
}

std::vector<std::unique_ptr<Stmt>> Parser::block() {
    std::vector<std::unique_ptr<Stmt>> statements;

    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        auto decl = declaration();
        if (decl) {
            statements.push_back(std::move(decl));
        }
    }

    consume(TokenType::RightBrace, "Expect '}' after block.");
    return statements;
}

std::unique_ptr<Stmt> Parser::expressionStatement() {
    std::unique_ptr<Expr> expr = expression();
    consume(TokenType::Semicolon, "Expect ';' after expression.");
    return std::make_unique<ExpressionStmt>(std::move(expr));
}

std::unique_ptr<Expr> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expr> Parser::assignment() {
    std::unique_ptr<Expr> expr = equality(); // might be GetExpr instead of VariableExpr. Assignment could work for property access too.

    if (match({TokenType::Equal})) {
        Token equals = previous();
        std::unique_ptr<Expr> value = assignment();

        if (auto* varExpr = dynamic_cast<VariableExpr*>(expr.get())) {
            Token name = varExpr->name;
            return std::make_unique<AssignExpr>(name, std::move(value));
        }
        // Could handle GetExpr here if we support objects/properties
        // if (GetExpr* getExpr = dynamic_cast<GetExpr*>(expr.get())) { ... }

        error(equals, "Invalid assignment target.");
    }

    return expr;
}

std::unique_ptr<Expr> Parser::equality() {
    std::unique_ptr<Expr> expr = comparison();

    while (match({TokenType::BangEqual, TokenType::EqualEqual})) {
        Token op = previous();
        std::unique_ptr<Expr> right = comparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::comparison() {
    std::unique_ptr<Expr> expr = term();

    while (match({TokenType::Greater, TokenType::GreaterEqual, TokenType::Less, TokenType::LessEqual})) {
        Token op = previous();
        std::unique_ptr<Expr> right = term();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::term() {
    std::unique_ptr<Expr> expr = factor();

    while (match({TokenType::Minus, TokenType::Plus})) {
        Token op = previous();
        std::unique_ptr<Expr> right = factor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::factor() {
    std::unique_ptr<Expr> expr = unary();

    while (match({TokenType::Slash, TokenType::Star})) {
        Token op = previous();
        std::unique_ptr<Expr> right = unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::unary() {
    if (match({TokenType::BangEqual /* Bang might be missing, adding it if needed */, TokenType::Minus})) {
        Token op = previous();
        std::unique_ptr<Expr> right = unary();
        return std::make_unique<UnaryExpr>(op, std::move(right));
    }

    return call();
}

std::unique_ptr<Expr> Parser::call() {
    std::unique_ptr<Expr> expr = primary();

    while (true) {
        if (match({TokenType::LeftParen})) {
            expr = finishCall(std::move(expr));
        } else if (match({TokenType::Dot})) {
            Token name = consume(TokenType::Identifier, "Expect property name after '.'.");
            expr = std::make_unique<GetExpr>(std::move(expr), name);
        } else {
            break;
        }
    }

    return expr;
}

std::unique_ptr<Expr> Parser::finishCall(std::unique_ptr<Expr> callee) {
    std::vector<std::unique_ptr<Expr>> arguments;
    if (!check(TokenType::RightParen)) {
        arguments.push_back(expression());
        while (match({TokenType::Comma})) {
            arguments.push_back(expression());
        }
    }

    Token paren = consume(TokenType::RightParen, "Expect ')' after arguments.");

    return std::make_unique<CallExpr>(std::move(callee), paren, std::move(arguments));
}

std::unique_ptr<Expr> Parser::primary() {
    if (match({TokenType::False})) {
        return std::make_unique<LiteralExpr>(false);
    }
    if (match({TokenType::True})) {
        return std::make_unique<LiteralExpr>(true);
    }

    if (match({TokenType::Number})) {
        return std::make_unique<LiteralExpr>(previous().numberValue);
    }

    if (match({TokenType::String})) {
        return std::make_unique<LiteralExpr>(previous().lexeme);
    }

    if (match({TokenType::Identifier})) {
        return std::make_unique<VariableExpr>(previous());
    }

    if (match({TokenType::LeftParen})) {
        std::unique_ptr<Expr> expr = expression();
        consume(TokenType::RightParen, "Expect ')' after expression.");
        return expr;
    }

    throw error(peek(), "Expect expression.");
}

bool Parser::match(std::initializer_list<TokenType> types) {
    if (std::ranges::any_of(types, [this](TokenType type) { return check(type); })) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) {
        return false;
    }
    return peek().type == type;
}

Token Parser::advance() {
    if (!isAtEnd()) {
        current++;
    }
    return previous();
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::Eof;
}

Token Parser::peek() const {
    return tokens[current];
}

Token Parser::previous() const {
    return tokens[current - 1];
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        return advance();
    }

    throw error(peek(), message);
}

ParseError Parser::error(const Token& token, const std::string& message) {
    // Ideally log the error somewhere
    return {token, message};
}

void Parser::synchronize() {
    advance();

    while (!isAtEnd()) {
        if (previous().type == TokenType::Semicolon) {
            return;
        }

        switch (peek().type) {
            case TokenType::Let:
            case TokenType::If:
            case TokenType::While:
                return;
            default:
                break;
        }

        advance();
    }
}

} // namespace System::Hardware::Scripting::Lang
