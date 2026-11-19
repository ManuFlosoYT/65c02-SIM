/*
 * dialog.c — Interactive TUI dialog demo for microDOS
 * Compile with: ./sdk/scripts/compile-bin.sh dialog --microDOS
 */
#include <stdbool.h>
#include <stdint.h>

#include "Libs/app-bios.h"
#include "Libs/tui.h"

int main(void) {
    tui_init();

    tui_set_color(TUI_COLOR_WHITE, TUI_COLOR_BLUE);
    tui_fill_rect(1, 1, 80, 25, ' ');

    tui_draw_header("Package configuration");

    const uint8_t w_row = 7;
    const uint8_t w_col = 20;
    const uint8_t w_width = 44;
    const uint8_t w_height = 12;

    tui_draw_window(w_row, w_col, w_width, w_height, "Configuration sun-java-jre");

    tui_set_color(TUI_COLOR_BLACK, TUI_COLOR_WHITE);
    tui_set_cursor(w_row + 2, w_col + 3);
    print_str("Bla bla bla...");

    tui_set_cursor(w_row + 4, w_col + 3);
    print_str("Do you accept?");

    tui_set_cursor(w_row + 8, w_col);
    print_str("\x1b[30;47m\x1b(0t");
    for (uint8_t i = 0; i < w_width - 2; ++i) {
        bios_putchar('q');
    }
    bios_putchar('u');
    print_str("\x1b(B");

    int selected = 0;
    bool running = true;

    while (running) {
        tui_draw_button(w_row + 9, w_col + 8, "Yes", selected == 0);
        tui_draw_button(w_row + 9, w_col + 24, "No", selected == 1);

        uint16_t key = tui_get_key();
        switch (key) {
            case TUI_KEY_LEFT:
            case TUI_KEY_RIGHT:
            case TUI_KEY_TAB:
            case TUI_KEY_SHIFT_TAB:
                selected = 1 - selected;
                break;
            case TUI_KEY_ENTER:
            case ' ':
            case 'y':
            case 'Y':
                if (key == 'y' || key == 'Y') {
                    selected = 0;
                }
                running = false;
                break;
            case 'n':
            case 'N':
                selected = 1;
                running = false;
                break;
            case TUI_KEY_ESC:
            case 'q':
            case 'Q':
                selected = -1;
                running = false;
                break;
            default:
                break;
        }
    }

    tui_shutdown();

    if (selected == 0) {
        println("Result: Accepted (Yes)");
    } else if (selected == 1) {
        println("Result: Rejected (No)");
    } else {
        println("Result: Cancelled");
    }

    return 0;
}
