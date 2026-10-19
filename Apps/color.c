#include <stdint.h>
#include <string.h>

#include "../Libs/app-bios.h"
#include "../Libs/tui.h"

int main(void) {
    char** args = _args_ptr;
    if (arg_count > 1 && strcmp(args[1], "test") == 0) {
        println("--- 256 Color Palette Test ---");
        for (uint16_t i = 0; i < 256; ++i) {
            tui_set_color_256(0, (uint8_t)i);
            bios_putchar(' ');
            if ((i + 1) % 16 == 0) {
                tui_reset_color();
                println("");
            }
        }
        tui_reset_color();
        println("--- RGB TrueColor Test ---");
        for (uint8_t i = 0; i < 32; ++i) {
            uint8_t val = (uint8_t)(i * 8);
            tui_set_color_rgb(val, 0, 255 - val, true);
            bios_putchar(' ');
        }
        tui_reset_color();
        println("");
        return 0;
    }

    if (arg_count >= 3 && strcmp(args[1], "default") == 0) {
        tui_set_default_colors(args[2], (arg_count > 3) ? args[3] : NULL);
        println("Default colors updated.");
        return 0;
    }

    if (arg_count >= 2 && strcmp(args[1], "reset") == 0) {
        print_str("\x1b]110\x07\x1b]111\x07\x1b[0m\x1b[2J\x1b[H");
        println("Default colors reset.");
        return 0;
    }

    println("Usage: color <test | default <fg_hex> [bg_hex] | reset>");
    return 0;
}

