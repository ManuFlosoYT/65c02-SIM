#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string_view>

#define FONT_MOCK_GPU 1
#include "sdk/microdos/Libs/Font5x5.h"

namespace {

constexpr size_t VRAM_WIDTH = 100;
constexpr size_t VRAM_HEIGHT = 64;

std::array<std::array<uint8_t, VRAM_WIDTH>, VRAM_HEIGHT> s_simulatedVram{};

void clear_simulated_vram() {
    for (auto& row : s_simulatedVram) {
        row.fill(0);
    }
}

void verify_glyph_at(int8_t originX, int8_t originY, char ch, uint8_t color) {
    ASSERT_GE(ch, 32);
    ASSERT_LE(ch, 126);
    const auto* glyph = FONT_5X5_GLYPHS[static_cast<uint8_t>(ch) - 32U];
    for (uint8_t r = 0; r < FONT_HEIGHT; ++r) {
        uint8_t rowBits = glyph[r];
        for (uint8_t c = 0; c < FONT_WIDTH; ++c) {
            bool expectedPixel = (rowBits & (0x10U >> c)) != 0;
            auto actualPixel = s_simulatedVram[static_cast<size_t>(originY + r)][static_cast<size_t>(originX + c)];
            EXPECT_EQ(actualPixel, expectedPixel ? color : 0)
                << "Mismatch for char '" << ch << "' at offset (" << static_cast<int>(c) << ", " << static_cast<int>(r) << ")";
        }
    }
}

} // namespace

extern "C" void gpu_put_pixel(int8_t x, int8_t y, uint8_t color) {
    if (x >= 0 && static_cast<size_t>(x) < VRAM_WIDTH && y >= 0 && static_cast<size_t>(y) < VRAM_HEIGHT) {
        s_simulatedVram[static_cast<size_t>(y)][static_cast<size_t>(x)] = color;
    }
}

TEST(Font5x5Test, GlyphBitmaps) {
    constexpr uint8_t testColor = 0x2A;

    clear_simulated_vram();
    font_draw_char(10, 10, ' ', testColor);
    for (size_t y = 10; y < 15; ++y) {
        for (size_t x = 10; x < 15; ++x) {
            EXPECT_EQ(s_simulatedVram[y][x], 0);
        }
    }

    clear_simulated_vram();
    font_draw_char(10, 10, '!', testColor);
    verify_glyph_at(10, 10, '!', testColor);

    clear_simulated_vram();
    font_draw_char(20, 20, '0', testColor);
    verify_glyph_at(20, 20, '0', testColor);

    clear_simulated_vram();
    font_draw_char(30, 30, 'A', testColor);
    verify_glyph_at(30, 30, 'A', testColor);

    clear_simulated_vram();
    font_draw_char(40, 40, 'z', testColor);
    verify_glyph_at(40, 40, 'z', testColor);
}

TEST(Font5x5Test, OutOfBoundsChars) {
    clear_simulated_vram();
    font_draw_char(10, 10, static_cast<char>(0), 0x3F);
    font_draw_char(10, 10, static_cast<char>(31), 0x3F);
    font_draw_char(10, 10, static_cast<char>(127), 0x3F);
    font_draw_char(10, 10, static_cast<char>(-1), 0x3F);
    font_draw_char(10, 10, static_cast<char>(-128), 0x3F);

    for (const auto& row : s_simulatedVram) {
        for (uint8_t pixel : row) {
            EXPECT_EQ(pixel, 0);
        }
    }
}

TEST(Font5x5Test, Clipping) {
    clear_simulated_vram();

    font_draw_char(-2, 10, 'A', 0x3F);
    EXPECT_EQ(s_simulatedVram[10][0], 0);
    EXPECT_EQ(s_simulatedVram[10][1], 0x3F);

    font_draw_char(-10, 10, 'A', 0x3F);
    font_draw_char(10, -2, 'A', 0x3F);
    font_draw_char(10, -10, 'A', 0x3F);

    font_draw_char(98, 10, 'A', 0x3F);
    EXPECT_EQ(s_simulatedVram[10][98], 0);
    EXPECT_EQ(s_simulatedVram[10][99], 0x3F);

    font_draw_char(105, 10, 'A', 0x3F);

    font_draw_char(10, 62, 'A', 0x3F);
    EXPECT_EQ(s_simulatedVram[62][11], 0x3F);

    font_draw_char(10, 70, 'A', 0x3F);
}

TEST(Font5x5Test, StringDrawingWithNewline) {
    clear_simulated_vram();
    font_draw_string(0, 0, nullptr, 0x3F);

    font_draw_string(10, 10, "A\nB", 0x3F);
    verify_glyph_at(10, 10, 'A', 0x3F);
    verify_glyph_at(10, 16, 'B', 0x3F);

    clear_simulated_vram();
    font_draw_string(0, 0, "AB", 0x3F);
    verify_glyph_at(0, 0, 'A', 0x3F);
    verify_glyph_at(6, 0, 'B', 0x3F);
    for (size_t y = 0; y < 5; ++y) {
        EXPECT_EQ(s_simulatedVram[y][5], 0);
    }
}

TEST(Font5x5Test, UnifiedFontDraw) {
    clear_simulated_vram();

    font_draw(0, 0, 'A');
    EXPECT_EQ(s_simulatedVram[0][1], FONT_DEFAULT_COLOR);

    font_draw(10, 0, 'A', 0x15);
    EXPECT_EQ(s_simulatedVram[0][11], 0x15);

    font_draw(0, 10, "A");
    EXPECT_EQ(s_simulatedVram[10][1], FONT_DEFAULT_COLOR);

    font_draw(10, 10, "A", 0x24);
    EXPECT_EQ(s_simulatedVram[10][11], 0x24);
}
