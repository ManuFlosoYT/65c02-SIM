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

TEST(ParserTest, ComplexExpression) {
    Lexer lexer("PC == $8000 && A > $10 || MEM[$0200] changed");
    auto tokens = lexer.scanTokens();
    Parser parser(tokens);
    auto expr = parser.parseExpression();

    ASSERT_NE(expr, nullptr);
    auto* orExpr = dynamic_cast<BinaryExpr*>(expr.get());
    ASSERT_NE(orExpr, nullptr);
    EXPECT_EQ(orExpr->op.type, TokenType::OrOr);

    auto* andExpr = dynamic_cast<BinaryExpr*>(orExpr->left.get());
    ASSERT_NE(andExpr, nullptr);
    EXPECT_EQ(andExpr->op.type, TokenType::AndAnd);

    auto* pcEq = dynamic_cast<BinaryExpr*>(andExpr->left.get());
    ASSERT_NE(pcEq, nullptr);
    EXPECT_EQ(pcEq->op.type, TokenType::EqualEqual);

    auto* aGt = dynamic_cast<BinaryExpr*>(andExpr->right.get());
    ASSERT_NE(aGt, nullptr);
    EXPECT_EQ(aGt->op.type, TokenType::Greater);

    auto* changedExpr = dynamic_cast<ChangedExpr*>(orExpr->right.get());
    ASSERT_NE(changedExpr, nullptr);

    auto* indexExpr = dynamic_cast<IndexExpr*>(changedExpr->expression.get());
    ASSERT_NE(indexExpr, nullptr);
    auto* memVar = dynamic_cast<VariableExpr*>(indexExpr->object.get());
    ASSERT_NE(memVar, nullptr);
    EXPECT_EQ(memVar->name.lexeme, "MEM");
    auto* addrLit = dynamic_cast<LiteralExpr*>(indexExpr->index.get());
    ASSERT_NE(addrLit, nullptr);
    EXPECT_EQ(std::get<int64_t>(addrLit->value), 0x0200);
}
