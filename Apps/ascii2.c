#include <stdint.h>

#include "Libs/app-bios.h"
#include "Libs/GPU.h"
#include "Libs/Font3x5.h"

int main(void) {
    const uint8_t colors[] = {0x3F, 0x30, 0x3C, 0x0C, 0x0F, 0x03, 0x33, 0x3E, 0x2D, 0x1F};

    gpu_fill_screen(0x00);
    println("--- microDOS ASCII 3x5 Font Demo ---");
    println("Screen 1: ASCII 3x5 Table Demo");
    println("Press any key to advance to Screen 2.");

    /* Screen 1: ASCII table demo */
    font3x5_draw(2, 2, "ASCII 3x5 TABLE (32-126)", 0x3F);
    gpu_draw_line(2, 8, 97, 8, 0x15);

    for (uint8_t r = 0; r < 6; ++r) {
        uint8_t color = colors[r % 10];
        for (uint8_t c = 0; c < 16; ++c) {
            uint8_t idx = (uint8_t)(r * 16 + c);
            if (idx >= 95) {
                break;
            }
            int8_t x = (int8_t)(c * 5 + 10);
            int8_t y = (int8_t)(r * 7 + 11);
            char ch = (char)(32 + idx);
            font3x5_draw(x, y, ch, color);
        }
    }

    gpu_draw_line(2, 53, 97, 53, 0x15);
    font3x5_draw(4, 56, "PRESS ANY KEY -> PAGE 2", 0x3E);

    bios_getchar();

    /* Screen 2: Specs, matrix comparison, technical considerations */
    gpu_fill_screen(0x00);
    println("\nScreen 2: 3x5 Matrix Specifications & Comparison");
    println("- Matrix: 3x5 font, 4x6 stride, 25x10 text grid");
    println("- Matrices: 3x3 (digits), 3x5 (min ASCII), 4x5, 5x5, 5x7");
    println("- Considerations: +56% density, 475B ROM, zero ambiguity");
    println("Press any key to exit.");

    font3x5_draw(2, 1,  "3x5 SPECS & COMPARISON",   0x3F);
    font3x5_draw(2, 7,  "CELL: 3x5  STRIDE: 4x6",    0x3E);
    font3x5_draw(2, 13, "3x3:DIGIT 3x5:MIN ASCII",   0x2D);
    font3x5_draw(2, 19, "4x5:HEAVY 5x5:microDOS",    0x2D);
    font3x5_draw(2, 25, "5x7:TELETYPE 12x8 GRID",    0x30);
    font3x5_draw(2, 32, "TECHNICAL NOTES:",          0x3C);
    font3x5_draw(2, 38, "+56% DENSITY (25x10)",      0x0F);
    font3x5_draw(2, 44, "ROM: 475B (5B / GLYPH)",    0x0F);
    font3x5_draw(2, 50, "UPPERCASE MAP: 0 AMBIG",    0x33);
    font3x5_draw(6, 57, "PRESS ANY KEY TO EXIT",     0x0C);

    bios_getchar();

    /* Clean exit restoring screen */
    gpu_fill_screen(0x00);
    println("Exiting ascii2.");
    return 0;
}
