#include <stdint.h>

#include "Libs/app-bios.h"
#include "Libs/GPU.h"
#include "Libs/Font5x5.h"

int main(void) {
    const uint8_t colors[] = {0x3F, 0x30, 0x3C, 0x0C, 0x0F, 0x03, 0x33, 0x3E, 0x2D, 0x1F};

    gpu_fill_screen(0x00);
    println("--- microDOS ASCII 5x5 Font Demo ---");
    println("Press any key to exit.");

    for (uint8_t r = 0; r < 10; ++r) {
        uint8_t color = colors[r % 10];
        for (uint8_t c = 0; c < 16; ++c) {
            int8_t x = (int8_t)(c * 6 + 2);
            int8_t y = (int8_t)(r * 6 + 2);
            char ch = (char)(32 + ((r * 16 + c) % 95));
            font_draw(x, y, ch, color);
        }
    }

    bios_getchar();
    gpu_fill_screen(0x00);
    return 0;
}
