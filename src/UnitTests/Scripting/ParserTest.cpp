#include <gtest/gtest.h>
#include "../../Hardware/Scripting/Lang/Lexer.h"
#include "../../Hardware/Scripting/Lang/Parser.h"

using namespace System::Hardware::Scripting::Lang;

TEST(ParserTest, VarDeclaration) {
    Lexer lexer("let x = 10;");
    auto tokens = lexer.scanTokens();
    Parser parser(tokens);
    auto stmts = parser.parse();

    ASSERT_EQ(stmts.size(), 1);
    VarStmt* varStmt = dynamic_cast<VarStmt*>(stmts[0].get());
    ASSERT_NE(varStmt, nullptr);
    EXPECT_EQ(varStmt->name.lexeme, "x");
    LiteralExpr* init = dynamic_cast<LiteralExpr*>(varStmt->initializer.get());
    ASSERT_NE(init, nullptr);
    EXPECT_EQ(std::get<int64_t>(init->value), 10);
}

TEST(ParserTest, IfStatement) {
    Lexer lexer("if (x < 5) { x = 0; } else { x = 1; }");
    auto tokens = lexer.scanTokens();
    Parser parser(tokens);
    auto stmts = parser.parse();

    ASSERT_EQ(stmts.size(), 1);
    IfStmt* ifStmt = dynamic_cast<IfStmt*>(stmts[0].get());
    ASSERT_NE(ifStmt, nullptr);
    ASSERT_NE(ifStmt->thenBranch.get(), nullptr);
    ASSERT_NE(ifStmt->elseBranch.get(), nullptr);
}

TEST(ParserTest, FunctionCall) {
    Lexer lexer("emu.pause();");
    auto tokens = lexer.scanTokens();
    Parser parser(tokens);
    auto stmts = parser.parse();

    ASSERT_EQ(stmts.size(), 1);
    ExpressionStmt* exprStmt = dynamic_cast<ExpressionStmt*>(stmts[0].get());
    ASSERT_NE(exprStmt, nullptr);
    
    CallExpr* callExpr = dynamic_cast<CallExpr*>(exprStmt->expression.get());
    ASSERT_NE(callExpr, nullptr);
    
    GetExpr* getExpr = dynamic_cast<GetExpr*>(callExpr->callee.get());
    ASSERT_NE(getExpr, nullptr);
    EXPECT_EQ(getExpr->name.lexeme, "pause");
    
    VariableExpr* varExpr = dynamic_cast<VariableExpr*>(getExpr->object.get());
    ASSERT_NE(varExpr, nullptr);
    EXPECT_EQ(varExpr->name.lexeme, "emu");
}
