#include "Interpreter.h"
#include <iostream>

namespace System::Hardware::Scripting::Lang {

Interpreter::Interpreter()
    : globals(std::make_shared<Environment>()), environment(globals) {}

void Interpreter::defineBuiltin(const std::string& name, BuiltinFunc func) {
    builtins[name] = std::move(func);
}

void Interpreter::interpret(const std::vector<std::unique_ptr<Stmt>>& statements) {
    try {
        for (const auto& statement : statements) {
            execute(statement.get());
        }
    } catch (RuntimeError& error) {
        std::string errStr = "Runtime Error: " + std::string(error.what()) + " [line " + std::to_string(error.token.line) + "]";
        if (printHandler) {
            printHandler(errStr);
        } else {
            std::cerr << errStr << "\n";
        }
    }
}

Value Interpreter::evaluate(Expr* expr) {
    if (auto* binary = dynamic_cast<BinaryExpr*>(expr)) {
        return visitBinaryExpr(binary);
    }
    if (auto* unary = dynamic_cast<UnaryExpr*>(expr)) {
        return visitUnaryExpr(unary);
    }
    if (auto* literal = dynamic_cast<LiteralExpr*>(expr)) {
        return visitLiteralExpr(literal);
    }
    if (auto* var = dynamic_cast<VariableExpr*>(expr)) {
        return visitVariableExpr(var);
    }
    if (auto* assign = dynamic_cast<AssignExpr*>(expr)) {
        return visitAssignExpr(assign);
    }
    if (auto* call = dynamic_cast<CallExpr*>(expr)) {
        return visitCallExpr(call);
    }
    if (auto* get = dynamic_cast<GetExpr*>(expr)) {
        return visitGetExpr(get);
    }

    throw std::runtime_error("Unknown expression type");
}

void Interpreter::execute(Stmt* stmt) {
    if (breakOut || continueLoop) {
        return; // Skip if breaking or continuing
    }

    if (auto* exprStmt = dynamic_cast<ExpressionStmt*>(stmt)) {
        visitExpressionStmt(exprStmt);
    } else if (auto* printStmt = dynamic_cast<PrintStmt*>(stmt)) {
        visitPrintStmt(printStmt);
    } else if (auto* varStmt = dynamic_cast<VarStmt*>(stmt)) {
        visitVarStmt(varStmt);
    } else if (auto* blockStmt = dynamic_cast<BlockStmt*>(stmt)) {
        visitBlockStmt(blockStmt);
    } else if (auto* ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        visitIfStmt(ifStmt);
    } else if (auto* whileStmt = dynamic_cast<WhileStmt*>(stmt)) {
        visitWhileStmt(whileStmt);
    } else if (auto* breakStmt = dynamic_cast<BreakStmt*>(stmt)) {
        visitBreakStmt(breakStmt);
    } else if (auto* continueStmt = dynamic_cast<ContinueStmt*>(stmt)) {
        visitContinueStmt(continueStmt);
    } else {
        throw std::runtime_error("Unknown statement type");
    }
}

void Interpreter::executeBlock(const std::vector<std::unique_ptr<Stmt>>& statements, std::shared_ptr<Environment> env) {
    std::shared_ptr<Environment> previous = this->environment;
    try {
        this->environment = std::move(env);
        for (const auto& statement : statements) {
            if (breakOut || continueLoop) {
                break;
            }
            execute(statement.get());
        }
    } catch (...) {
        this->environment = previous;
        throw;
    }
    this->environment = previous;
}

Value Interpreter::visitBinaryExpr(BinaryExpr* expr) {
    Value left = evaluate(expr->left.get());
    Value right = evaluate(expr->right.get());

    switch (expr->op.type) {
        case TokenType::Greater:
            checkNumberOperands(expr->op, left, right);
            return std::get<int64_t>(left) > std::get<int64_t>(right);
        case TokenType::GreaterEqual:
            checkNumberOperands(expr->op, left, right);
            return std::get<int64_t>(left) >= std::get<int64_t>(right);
        case TokenType::Less:
            checkNumberOperands(expr->op, left, right);
            return std::get<int64_t>(left) < std::get<int64_t>(right);
        case TokenType::LessEqual:
            checkNumberOperands(expr->op, left, right);
            return std::get<int64_t>(left) <= std::get<int64_t>(right);
        case TokenType::BangEqual: return !isEqual(left, right);
        case TokenType::EqualEqual: return isEqual(left, right);
        case TokenType::Minus:
            checkNumberOperands(expr->op, left, right);
            return std::get<int64_t>(left) - std::get<int64_t>(right);
        case TokenType::Plus:
            if (std::holds_alternative<int64_t>(left) && std::holds_alternative<int64_t>(right)) {
                return std::get<int64_t>(left) + std::get<int64_t>(right);
            }
            if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right)) {
                return std::get<std::string>(left) + std::get<std::string>(right);
            }
            throw RuntimeError(expr->op, "Operands must be two numbers or two strings.");
        case TokenType::Slash:
            checkNumberOperands(expr->op, left, right);
            if (std::get<int64_t>(right) == 0) {
                throw RuntimeError(expr->op, "Division by zero.");
            }
            return std::get<int64_t>(left) / std::get<int64_t>(right);
        case TokenType::Star:
            checkNumberOperands(expr->op, left, right);
            return std::get<int64_t>(left) * std::get<int64_t>(right);
        default: break;
    }

    return std::monostate{};
}

Value Interpreter::visitUnaryExpr(UnaryExpr* expr) {
    Value right = evaluate(expr->right.get());

    switch (expr->op.type) {
        case TokenType::BangEqual: // ! usually
            return !isTruthy(right);
        case TokenType::Minus:
            checkNumberOperand(expr->op, right);
            return -std::get<int64_t>(right);
        default: break;
    }

    return std::monostate{};
}

Value Interpreter::visitLiteralExpr(LiteralExpr* expr) {
    return std::visit([](auto&& arg) -> Value { return arg; }, expr->value);
}

Value Interpreter::visitVariableExpr(VariableExpr* expr) {
    return environment->get(expr->name);
}

Value Interpreter::visitAssignExpr(AssignExpr* expr) {
    Value value = evaluate(expr->value.get());
    environment->assign(expr->name, value);
    return value;
}

Value Interpreter::visitCallExpr(CallExpr* expr) {
    Value callee = evaluate(expr->callee.get());

    std::vector<Value> arguments;
    arguments.reserve(expr->arguments.size());
    for (const auto& arg : expr->arguments) {
        arguments.push_back(evaluate(arg.get()));
    }

    if (std::holds_alternative<Callable>(callee)) {
        std::string funcName = std::get<Callable>(callee).name;
        if (builtins.contains(funcName)) {
            return builtins[funcName](arguments);
        }
        throw RuntimeError(expr->paren, "Unknown builtin function '" + funcName + "'.");
    }

    throw RuntimeError(expr->paren, "Can only call functions and classes.");
}

Value Interpreter::visitGetExpr(GetExpr* expr) {
    if (auto* varExpr = dynamic_cast<VariableExpr*>(expr->object.get())) {
        if (varExpr->name.lexeme == "emu") {
            return Callable{"emu." + expr->name.lexeme};
        }
    }
    
    Value object = evaluate(expr->object.get());
    throw RuntimeError(expr->name, "Property access not supported on this object.");
}

static std::string stringify(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) {
        return "nil";
    }
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    }
    if (std::holds_alternative<int64_t>(value)) {
        return std::to_string(std::get<int64_t>(value));
    }
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    }
    if (std::holds_alternative<Callable>(value)) {
        return "<fn " + std::get<Callable>(value).name + ">";
    }
    return "";
}

void Interpreter::visitExpressionStmt(ExpressionStmt* stmt) {
    evaluate(stmt->expression.get());
}

void Interpreter::visitPrintStmt(PrintStmt* stmt) {
    Value value = evaluate(stmt->expression.get());
    std::string text = stringify(value);
    if (printHandler) {
        printHandler(text);
    } else {
        std::cout << text << "\n";
    }
}

void Interpreter::visitVarStmt(VarStmt* stmt) {
    Value value = std::monostate{};
    if (stmt->initializer != nullptr) {
        value = evaluate(stmt->initializer.get());
    }

    environment->define(stmt->name.lexeme, value);
}

void Interpreter::visitBlockStmt(BlockStmt* stmt) {
    executeBlock(stmt->statements, std::make_shared<Environment>(environment));
}

void Interpreter::visitIfStmt(IfStmt* stmt) {
    if (isTruthy(evaluate(stmt->condition.get()))) {
        execute(stmt->thenBranch.get());
    } else if (stmt->elseBranch != nullptr) {
        execute(stmt->elseBranch.get());
    }
}

void Interpreter::visitWhileStmt(WhileStmt* stmt) {
    while (isTruthy(evaluate(stmt->condition.get()))) {
        execute(stmt->body.get());
        
        if (breakOut) {
            breakOut = false;
            break;
        }
        if (continueLoop) {
            continueLoop = false;
        }
    }
}

void Interpreter::visitBreakStmt(BreakStmt* stmt) {
    breakOut = true;
}

void Interpreter::visitContinueStmt(ContinueStmt* stmt) {
    continueLoop = true;
}

bool Interpreter::isTruthy(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) {
        return false;
    }
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value);
    }
    if (std::holds_alternative<int64_t>(value)) {
        return std::get<int64_t>(value) != 0;
    }
    return true; // Empty string is truthy in this logic, adjust if necessary
}

bool Interpreter::isEqual(const Value& a, const Value& b) {
    if (a.index() != b.index()) {
        return false;
    }
    if (std::holds_alternative<std::monostate>(a)) {
        return true;
    }
    if (std::holds_alternative<bool>(a)) {
        return std::get<bool>(a) == std::get<bool>(b);
    }
    if (std::holds_alternative<int64_t>(a)) {
        return std::get<int64_t>(a) == std::get<int64_t>(b);
    }
    if (std::holds_alternative<std::string>(a)) {
        return std::get<std::string>(a) == std::get<std::string>(b);
    }
    return false;
}

void Interpreter::checkNumberOperand(const Token& op, const Value& operand) {
    if (std::holds_alternative<int64_t>(operand)) {
        return;
    }
    throw RuntimeError(op, "Operand must be a number.");
}

void Interpreter::checkNumberOperands(const Token& op, const Value& left, const Value& right) {
    if (std::holds_alternative<int64_t>(left) && std::holds_alternative<int64_t>(right)) {
        return;
    }
    throw RuntimeError(op, "Operands must be numbers.");
}

} // namespace System::Hardware::Scripting::Lang
