#pragma once

#include "AST.h"
#include "Environment.h"
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>

namespace System::Hardware::Scripting::Lang {

using BuiltinFunc = std::function<Value(const std::vector<Value>&)>;

class Interpreter {
public:
    Interpreter();

    void interpret(const std::vector<std::unique_ptr<Stmt>>& statements);
    void defineBuiltin(const std::string& name, BuiltinFunc func);

    
    void setPrintHandler(std::function<void(const std::string&)> handler) {
        printHandler = std::move(handler);
    }

   private:
    std::function<void(const std::string&)> printHandler;
    Value evaluate(Expr* expr);
    void execute(Stmt* stmt);
    void executeBlock(const std::vector<std::unique_ptr<Stmt>>& statements, std::shared_ptr<Environment> environment);

    Value visitBinaryExpr(BinaryExpr* expr);
    Value visitUnaryExpr(UnaryExpr* expr);
    static Value visitLiteralExpr(LiteralExpr* expr);
    Value visitVariableExpr(VariableExpr* expr);
    Value visitAssignExpr(AssignExpr* expr);
    Value visitCallExpr(CallExpr* expr);
    Value visitGetExpr(GetExpr* expr);

    void visitExpressionStmt(ExpressionStmt* stmt);
    void visitPrintStmt(PrintStmt* stmt);
    void visitVarStmt(VarStmt* stmt);
    void visitBlockStmt(BlockStmt* stmt);
    void visitIfStmt(IfStmt* stmt);
    void visitWhileStmt(WhileStmt* stmt);
    void visitBreakStmt(BreakStmt* stmt);
    void visitContinueStmt(ContinueStmt* stmt);

    static bool isTruthy(const Value& value);
    static bool isEqual(const Value& a, const Value& b);
    static void checkNumberOperand(const Token& op, const Value& operand);
    static void checkNumberOperands(const Token& op, const Value& left, const Value& right);

    std::shared_ptr<Environment> globals;
    std::shared_ptr<Environment> environment;
    std::unordered_map<std::string, BuiltinFunc> builtins;

    bool breakOut = false;
    bool continueLoop = false;
};

} // namespace System::Hardware::Scripting::Lang
