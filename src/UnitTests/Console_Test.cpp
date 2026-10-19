#include "Frontend/Control/Console.h"

#include <gtest/gtest.h>

class Console_Test : public ::testing::Test {
   protected:
    void SetUp() override { Console::Clear(); }

    static void SendString(const std::string& str) {
        for (char c : str) {
            Console::OutputCallback(c);
        }
    }
};

TEST_F(Console_Test, StandardAnsiColors) {
    SendString("\x1b[31mA");
    const auto& cellRed = Console::GetCell(0, 0);
    EXPECT_EQ(cellRed.codepoint, 'A');
    EXPECT_EQ(cellRed.fg, 0xFF2B31CD);

    SendString("\x1b[42mB");
    const auto& cellGreenBg = Console::GetCell(1, 0);
    EXPECT_EQ(cellGreenBg.codepoint, 'B');
    EXPECT_EQ(cellGreenBg.bg, 0xFF79BC0D);

    SendString("\x1b[94mC");
    const auto& cellBrightBlue = Console::GetCell(2, 0);
    EXPECT_EQ(cellBrightBlue.codepoint, 'C');
    EXPECT_EQ(cellBrightBlue.fg, 0xFFEA8E3B);
}

TEST_F(Console_Test, Extended256Colors) {
    // 256-color palette index 196: RGB (255, 0, 0) -> 0xFF0000FF
    SendString("\x1b[38;5;196mX");
    const auto& cellFg256 = Console::GetCell(0, 0);
    EXPECT_EQ(cellFg256.codepoint, 'X');
    EXPECT_EQ(cellFg256.fg, 0xFF0000FF);

    // 256-color palette index 21: RGB (0, 0, 255) -> 0xFFFF0000
    SendString("\x1b[48;5;21mY");
    const auto& cellBg256 = Console::GetCell(1, 0);
    EXPECT_EQ(cellBg256.codepoint, 'Y');
    EXPECT_EQ(cellBg256.bg, 0xFFFF0000);

    // 256-color grayscale index 240: value = 8 + 10*(240-232) = 88 (0x58) -> 0xFF585858
    SendString("\x1b[38;5;240mZ");
    const auto& cellGray = Console::GetCell(2, 0);
    EXPECT_EQ(cellGray.codepoint, 'Z');
    EXPECT_EQ(cellGray.fg, 0xFF585858);
}

TEST_F(Console_Test, TrueColorRGB) {
    // TrueColor FG: R=255, G=128, B=64 -> 0xFF4080FF
    // TrueColor BG: R=10, G=20, B=30 -> 0xFF1E140A
    SendString("\x1b[38;2;255;128;64;48;2;10;20;30mT");
    const auto& cellRgb = Console::GetCell(0, 0);
    EXPECT_EQ(cellRgb.codepoint, 'T');
    EXPECT_EQ(cellRgb.fg, 0xFF4080FF);
    EXPECT_EQ(cellRgb.bg, 0xFF1E140A);

    // Multi-attribute combo: bold + TrueColor + underline
    SendString("\x1b[1;38;2;200;100;50;4mU");
    const auto& cellCombo = Console::GetCell(1, 0);
    EXPECT_EQ(cellCombo.codepoint, 'U');
    EXPECT_EQ(cellCombo.fg, 0xFF3264C8);
    EXPECT_TRUE((cellCombo.attributes & Console::ATTR_BOLD) != 0);
    EXPECT_TRUE((cellCombo.attributes & Console::ATTR_UNDERLINE) != 0);
}

TEST_F(Console_Test, OscDefaultColorCommands) {
    // Set default FG to #FF8800 (R=255, G=136, B=0) -> 0xFF0088FF
    SendString("\x1b]10;#FF8800\x07");
    EXPECT_EQ(Console::GetDefaultFg(), 0xFF0088FF);

    // Set default BG to #000080 (R=0, G=0, B=128) -> 0xFF800000
    SendString("\x1b]11;#000080\x07");
    EXPECT_EQ(Console::GetDefaultBg(), 0xFF800000);

    // Clear and print character: should use updated default FG and BG
    SendString("\x1b[0mM");
    const auto& cell = Console::GetCell(0, 0);
    EXPECT_EQ(cell.fg, 0xFF0088FF);
    EXPECT_EQ(cell.bg, 0xFF800000);

    // Reset default FG and BG via OSC 110 / 111
    SendString("\x1b]110\x07");
    EXPECT_EQ(Console::GetDefaultFg(), Console::DEFAULT_FG);

    SendString("\x1b]111\x07");
    EXPECT_EQ(Console::GetDefaultBg(), Console::DEFAULT_BG);
}
