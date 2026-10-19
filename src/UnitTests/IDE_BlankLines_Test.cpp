#include <gtest/gtest.h>

#include <string>

#include "Frontend/GUI/IDEUtils.h"

TEST(IDEBlankLinesTest, MultipleTrailingBlankLinesWithSpaces) {
    std::string code = "int main() {\n    return 0;\n}\n    \n    \n";
    GUI::StripSpacesFromTrailingBlankLines(code);
    EXPECT_EQ(code, "int main() {\n    return 0;\n}\n\n\n");
}

TEST(IDEBlankLinesTest, TrailingBlankLineWithoutTrailingNewline) {
    std::string code = "int a;\n    ";
    GUI::StripSpacesFromTrailingBlankLines(code);
    EXPECT_EQ(code, "int a;\n");
}

TEST(IDEBlankLinesTest, WindowsCRLFLineEndings) {
    std::string code = "int a;\r\n    \r\n";
    GUI::StripSpacesFromTrailingBlankLines(code);
    EXPECT_EQ(code, "int a;\r\n\r\n");
}

TEST(IDEBlankLinesTest, BlankLineInMiddleOfCodeNotTouched) {
    std::string code = "int a;\n    \nint b;\n    \n";
    GUI::StripSpacesFromTrailingBlankLines(code);
    EXPECT_EQ(code, "int a;\n    \nint b;\n\n");
}

TEST(IDEBlankLinesTest, EmptyTextOrWithoutTrailingSpaces) {
    std::string empty = "";
    GUI::StripSpacesFromTrailingBlankLines(empty);
    EXPECT_EQ(empty, "");

    std::string code = "int a;\n";
    GUI::StripSpacesFromTrailingBlankLines(code);
    EXPECT_EQ(code, "int a;\n");
}

TEST(StripTrailingNewlineTest, StripsSingleUnixNewline) {
    std::string code = "int a;\n";
    GUI::StripTrailingNewline(code);
    EXPECT_EQ(code, "int a;");
}

TEST(StripTrailingNewlineTest, StripsSingleWindowsNewline) {
    std::string code = "int a;\r\n";
    GUI::StripTrailingNewline(code);
    EXPECT_EQ(code, "int a;");
}

TEST(StripTrailingNewlineTest, PreservesIntentionalBlankLine) {
    std::string code = "int a;\n\n";
    GUI::StripTrailingNewline(code);
    EXPECT_EQ(code, "int a;\n");
}

TEST(StripTrailingNewlineTest, UnchangedWithoutTrailingNewline) {
    std::string code = "int a;";
    GUI::StripTrailingNewline(code);
    EXPECT_EQ(code, "int a;");
}

TEST(StripTrailingNewlineTest, EmptyString) {
    std::string code = "";
    GUI::StripTrailingNewline(code);
    EXPECT_EQ(code, "");
}

