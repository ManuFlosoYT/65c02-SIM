#pragma once

#include "Token.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <variant>

namespace System::Hardware::Scripting::Lang {

enum class ExprKind : uint8_t {
    Binary,
    Unary,
    Literal,
    Variable,
    Assign,
    Call,
    Get,
    Index,
    Changed
};

enum class StmtKind : uint8_t {
    Expression,
    Print,
    Var,
    Block,
    If,
    While,
    Break,
    Continue
};

struct Expr {
    const ExprKind kind;
    explicit Expr(ExprKind k) : kind(k) {}
    virtual ~Expr() = default;
    Expr(const Expr&) = delete;
    Expr& operator=(const Expr&) = delete;
    Expr(Expr&&) = delete;
    Expr& operator=(Expr&&) = delete;
};

struct Stmt {
    const StmtKind kind;
    explicit Stmt(StmtKind k) : kind(k) {}
    virtual ~Stmt() = default;
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;
    Stmt(Stmt&&) = delete;
    Stmt& operator=(Stmt&&) = delete;
};

// Expressions

struct BinaryExpr : public Expr {
    std::unique_ptr<Expr> left;
    Token op;
    std::unique_ptr<Expr> right;

    BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
        : Expr(ExprKind::Binary), left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
};

struct UnaryExpr : public Expr {
    Token op;
    std::unique_ptr<Expr> right;

    UnaryExpr(Token op, std::unique_ptr<Expr> right)
        : Expr(ExprKind::Unary), op(std::move(op)), right(std::move(right)) {}
};

struct LiteralExpr : public Expr {
    std::variant<std::monostate, int64_t, std::string, bool> value; // monostate for null/nil
    
    explicit LiteralExpr(int64_t v) : Expr(ExprKind::Literal), value(v) {}
    explicit LiteralExpr(std::string v) : Expr(ExprKind::Literal), value(std::move(v)) {}
    explicit LiteralExpr(bool v) : Expr(ExprKind::Literal), value(v) {}
};

struct VariableExpr : public Expr {
    Token name;

    explicit VariableExpr(Token name) : Expr(ExprKind::Variable), name(std::move(name)) {}
};

struct AssignExpr : public Expr {
    Token name;
    std::unique_ptr<Expr> value;

    AssignExpr(Token name, std::unique_ptr<Expr> value)
        : Expr(ExprKind::Assign), name(std::move(name)), value(std::move(value)) {}
};

struct CallExpr : public Expr {
    std::unique_ptr<Expr> callee;
    Token paren; // For error reporting
    std::vector<std::unique_ptr<Expr>> arguments;

    CallExpr(std::unique_ptr<Expr> callee, Token paren, std::vector<std::unique_ptr<Expr>> arguments)
        : Expr(ExprKind::Call), callee(std::move(callee)), paren(std::move(paren)), arguments(std::move(arguments)) {}
};

struct GetExpr : public Expr {
    std::unique_ptr<Expr> object;
    Token name;

    GetExpr(std::unique_ptr<Expr> object, Token name)
        : Expr(ExprKind::Get), object(std::move(object)), name(std::move(name)) {}
};

struct IndexExpr : public Expr {
    std::unique_ptr<Expr> object;
    Token bracket;
    std::unique_ptr<Expr> index;

    IndexExpr(std::unique_ptr<Expr> object, Token bracket, std::unique_ptr<Expr> index)
        : Expr(ExprKind::Index), object(std::move(object)), bracket(std::move(bracket)), index(std::move(index)) {}
};

struct ChangedExpr : public Expr {
    std::unique_ptr<Expr> expression;
    Token keyword;

    ChangedExpr(std::unique_ptr<Expr> expression, Token keyword)
        : Expr(ExprKind::Changed), expression(std::move(expression)), keyword(std::move(keyword)) {}
};

// Statements

struct ExpressionStmt : public Stmt {
    std::unique_ptr<Expr> expression;

    explicit ExpressionStmt(std::unique_ptr<Expr> expression)
        : Stmt(StmtKind::Expression), expression(std::move(expression)) {}
};

struct PrintStmt : public Stmt { // Optional: useful for debugging
    std::unique_ptr<Expr> expression;

    explicit PrintStmt(std::unique_ptr<Expr> expression)
        : Stmt(StmtKind::Print), expression(std::move(expression)) {}
};

struct VarStmt : public Stmt {
    Token name;
    std::unique_ptr<Expr> initializer;

    VarStmt(Token name, std::unique_ptr<Expr> initializer)
        : Stmt(StmtKind::Var), name(std::move(name)), initializer(std::move(initializer)) {}
};

struct BlockStmt : public Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;

    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> statements)
        : Stmt(StmtKind::Block), statements(std::move(statements)) {}
};

struct IfStmt : public Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;

    IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> thenBranch, std::unique_ptr<Stmt> elseBranch)
        : Stmt(StmtKind::If), condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
};

struct WhileStmt : public Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;

    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
        : Stmt(StmtKind::While), condition(std::move(condition)), body(std::move(body)) {}
};

struct BreakStmt : public Stmt {
    BreakStmt() : Stmt(StmtKind::Break) {}
};

struct ContinueStmt : public Stmt {
    ContinueStmt() : Stmt(StmtKind::Continue) {}
};

} // namespace System::Hardware::Scripting::Lang
