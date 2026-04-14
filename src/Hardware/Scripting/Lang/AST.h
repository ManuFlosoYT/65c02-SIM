#pragma once

#include "Token.h"
#include <memory>
#include <vector>
#include <string>

namespace System::Hardware::Scripting::Lang {

struct Expr {
    virtual ~Expr() = default;
};

struct Stmt {
    virtual ~Stmt() = default;
};

// Expressions

struct BinaryExpr : public Expr {
    std::unique_ptr<Expr> left;
    Token op;
    std::unique_ptr<Expr> right;

    BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
};

struct UnaryExpr : public Expr {
    Token op;
    std::unique_ptr<Expr> right;

    UnaryExpr(Token op, std::unique_ptr<Expr> right)
        : op(std::move(op)), right(std::move(right)) {}
};

struct LiteralExpr : public Expr {
    std::variant<std::monostate, int64_t, std::string, bool> value; // monostate for null/nil
    
    explicit LiteralExpr(int64_t v) : value(v) {}
    explicit LiteralExpr(std::string v) : value(std::move(v)) {}
    explicit LiteralExpr(bool v) : value(v) {}
    LiteralExpr() : value(std::monostate{}) {}
};

struct VariableExpr : public Expr {
    Token name;

    explicit VariableExpr(Token name) : name(std::move(name)) {}
};

struct AssignExpr : public Expr {
    Token name;
    std::unique_ptr<Expr> value;

    AssignExpr(Token name, std::unique_ptr<Expr> value)
        : name(std::move(name)), value(std::move(value)) {}
};

struct CallExpr : public Expr {
    std::unique_ptr<Expr> callee;
    Token paren; // For error reporting
    std::vector<std::unique_ptr<Expr>> arguments;

    CallExpr(std::unique_ptr<Expr> callee, Token paren, std::vector<std::unique_ptr<Expr>> arguments)
        : callee(std::move(callee)), paren(std::move(paren)), arguments(std::move(arguments)) {}
};

struct GetExpr : public Expr {
    std::unique_ptr<Expr> object;
    Token name;

    GetExpr(std::unique_ptr<Expr> object, Token name)
        : object(std::move(object)), name(std::move(name)) {}
};

// Statements

struct ExpressionStmt : public Stmt {
    std::unique_ptr<Expr> expression;

    explicit ExpressionStmt(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}
};

struct PrintStmt : public Stmt { // Optional: useful for debugging
    std::unique_ptr<Expr> expression;

    explicit PrintStmt(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}
};

struct VarStmt : public Stmt {
    Token name;
    std::unique_ptr<Expr> initializer;

    VarStmt(Token name, std::unique_ptr<Expr> initializer)
        : name(std::move(name)), initializer(std::move(initializer)) {}
};

struct BlockStmt : public Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;

    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> statements)
        : statements(std::move(statements)) {}
};

struct IfStmt : public Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;

    IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> thenBranch, std::unique_ptr<Stmt> elseBranch)
        : condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
};

struct WhileStmt : public Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;

    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
        : condition(std::move(condition)), body(std::move(body)) {}
};

struct BreakStmt : public Stmt {
    Token keyword;
    explicit BreakStmt(Token keyword) : keyword(std::move(keyword)) {}
};

struct ContinueStmt : public Stmt {
    Token keyword;
    explicit ContinueStmt(Token keyword) : keyword(std::move(keyword)) {}
};

} // namespace System::Hardware::Scripting::Lang
