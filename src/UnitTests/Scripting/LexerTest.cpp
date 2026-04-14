#include <gtest/gtest.h>
#include "../../Hardware/Scripting/Lang/Lexer.h"

using namespace System::Hardware::Scripting::Lang;

TEST(LexerTest, Keywords) {
    Lexer lexer("let if else while break continue true false");
    auto tokens = lexer.scanTokens();
    
    ASSERT_EQ(tokens.size(), 9); // 8 keywords + EOF
    EXPECT_EQ(tokens[0].type, TokenType::Let);
    EXPECT_EQ(tokens[1].type, TokenType::If);
    EXPECT_EQ(tokens[2].type, TokenType::Else);
    EXPECT_EQ(tokens[3].type, TokenType::While);
    EXPECT_EQ(tokens[4].type, TokenType::Break);
    EXPECT_EQ(tokens[5].type, TokenType::Continue);
    EXPECT_EQ(tokens[6].type, TokenType::True);
    EXPECT_EQ(tokens[7].type, TokenType::False);
    EXPECT_EQ(tokens[8].type, TokenType::Eof);
}

TEST(LexerTest, Numbers) {
    Lexer lexer("123 0x1A");
    auto tokens = lexer.scanTokens();
    
    ASSERT_EQ(tokens.size(), 3); // 2 numbers + EOF
    EXPECT_EQ(tokens[0].type, TokenType::Number);
    EXPECT_EQ(tokens[0].numberValue, 123);
    EXPECT_EQ(tokens[1].type, TokenType::Number);
    EXPECT_EQ(tokens[1].numberValue, 26);
}

TEST(LexerTest, Identifiers) {
    Lexer lexer("emu_val var1");
    auto tokens = lexer.scanTokens();
    
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::Identifier);
    EXPECT_EQ(tokens[0].lexeme, "emu_val");
    EXPECT_EQ(tokens[1].type, TokenType::Identifier);
    EXPECT_EQ(tokens[1].lexeme, "var1");
}

TEST(LexerTest, Symbols) {
    Lexer lexer("= == != < <= > >= + - * / () {} [],.");
    auto tokens = lexer.scanTokens();
    
    // There are 19 symbols here, plus EOF
    ASSERT_EQ(tokens.size(), 20);
    EXPECT_EQ(tokens[0].type, TokenType::Equal);
    EXPECT_EQ(tokens[1].type, TokenType::EqualEqual);
    EXPECT_EQ(tokens[2].type, TokenType::BangEqual);
    EXPECT_EQ(tokens[3].type, TokenType::Less);
    EXPECT_EQ(tokens[4].type, TokenType::LessEqual);
    EXPECT_EQ(tokens[5].type, TokenType::Greater);
    EXPECT_EQ(tokens[6].type, TokenType::GreaterEqual);
    EXPECT_EQ(tokens[7].type, TokenType::Plus);
    EXPECT_EQ(tokens[8].type, TokenType::Minus);
    EXPECT_EQ(tokens[9].type, TokenType::Star);
    EXPECT_EQ(tokens[10].type, TokenType::Slash);
    EXPECT_EQ(tokens[11].type, TokenType::LeftParen);
    EXPECT_EQ(tokens[12].type, TokenType::RightParen);
    EXPECT_EQ(tokens[13].type, TokenType::LeftBrace);
    EXPECT_EQ(tokens[14].type, TokenType::RightBrace);
    EXPECT_EQ(tokens[15].type, TokenType::LeftBracket);
    EXPECT_EQ(tokens[16].type, TokenType::RightBracket);
    EXPECT_EQ(tokens[17].type, TokenType::Comma);
    EXPECT_EQ(tokens[18].type, TokenType::Dot);
}
