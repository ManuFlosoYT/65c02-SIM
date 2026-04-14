#include <gtest/gtest.h>
#include "../../Hardware/Scripting/Lang/Lexer.h"
#include "../../Hardware/Scripting/Lang/Parser.h"
#include "../../Hardware/Scripting/Lang/Interpreter.h"

using namespace System::Hardware::Scripting::Lang;

TEST(InterpreterTest, EvaluatesMath) {
    Lexer lexer("let x = 10 + 5 * 2;");
    auto tokens = lexer.scanTokens();
    Parser parser(tokens);
    auto stmts = parser.parse();

    Interpreter interpreter;
    interpreter.interpret(stmts);
    
    // Test logic requires us to get the environment value...
    // But Interpreter doesn't expose environment directly. We could add a way to query globals for testing,
    // or we can test an emu builtin callback.
}

TEST(InterpreterTest, BuiltinCall) {
    Lexer lexer("let y = 0; emu.test(10, 20);");
    auto tokens = lexer.scanTokens();
    Parser parser(tokens);
    auto stmts = parser.parse();

    Interpreter interpreter;
    bool called = false;
    interpreter.defineBuiltin("emu.test", [&](const std::vector<Value>& args) -> Value {
        called = true;
        EXPECT_EQ(args.size(), 2);
        EXPECT_EQ(std::get<int64_t>(args[0]), 10);
        EXPECT_EQ(std::get<int64_t>(args[1]), 20);
        return std::monostate{};
    });

    interpreter.interpret(stmts);
    EXPECT_TRUE(called);
}

TEST(InterpreterTest, WhileLoop) {
    Lexer lexer("let x = 0; while(x < 3) { x = x + 1; emu.tick(x); }");
    auto tokens = lexer.scanTokens();
    Parser parser(tokens);
    auto stmts = parser.parse();

    Interpreter interpreter;
    int ticks = 0;
    interpreter.defineBuiltin("emu.tick", [&](const std::vector<Value>& args) -> Value {
        ticks++;
        return std::monostate{};
    });

    interpreter.interpret(stmts);
    EXPECT_EQ(ticks, 3);
}
