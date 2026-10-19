#ifndef TUI_H
#define TUI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Libs/app-bios.h"

#define TUI_COLOR_BLACK 0
#define TUI_COLOR_RED 1
#define TUI_COLOR_GREEN 2
#define TUI_COLOR_YELLOW 3
#define TUI_COLOR_BLUE 4
#define TUI_COLOR_MAGENTA 5
#define TUI_COLOR_CYAN 6
#define TUI_COLOR_WHITE 7
#define TUI_COLOR_DEFAULT 9

typedef enum {
    TUI_KEY_NONE = 0,
    TUI_KEY_ENTER = 10,
    TUI_KEY_TAB = 9,
    TUI_KEY_ESC = 27,
    TUI_KEY_SPACE = 32,
    TUI_KEY_UP = 256,
    TUI_KEY_DOWN,
    TUI_KEY_RIGHT,
    TUI_KEY_LEFT,
    TUI_KEY_SHIFT_TAB,
    TUI_KEY_HOME,
    TUI_KEY_END,
    TUI_KEY_DELETE
} tui_key_t;

static inline void tui_init(void) { print_str("\x1b[0m\x1b[?1049h\x1b[?25l\x1b[2J\x1b[H"); }

static inline void tui_shutdown(void) { print_str("\x1b[0m\x1b[?25h\x1b[?1049l"); }

static inline void tui_clear_screen(void) { print_str("\x1b[2J\x1b[H"); }

static inline void tui_set_cursor(uint8_t row, uint8_t col) {
    print_str("\x1b[");
    print_num(row);
    bios_putchar(';');
    print_num(col);
    bios_putchar('H');
}

static inline void tui_set_color(uint8_t fg, uint8_t bg) {
    print_str("\x1b[3");
    bios_putchar('0' + fg);
    print_str(";4");
    bios_putchar('0' + bg);
    bios_putchar('m');
}

static inline void tui_set_color_256(uint8_t fg, uint8_t bg) {
    print_str("\x1b[38;5;");
    print_num(fg);
    print_str(";48;5;");
    print_num(bg);
    bios_putchar('m');
}

static inline void tui_set_color_rgb(uint8_t r, uint8_t g, uint8_t b, bool is_bg) {
    print_str(is_bg ? "\x1b[48;2;" : "\x1b[38;2;");
    print_num(r);
    bios_putchar(';');
    print_num(g);
    bios_putchar(';');
    print_num(b);
    bios_putchar('m');
}

static inline void tui_set_default_colors(const char* fg_hex, const char* bg_hex) {
    if (fg_hex != NULL) {
        print_str("\x1b]10;#");
        print_str(fg_hex);
        bios_putchar(7);
    }
    if (bg_hex != NULL) {
        print_str("\x1b]11;#");
        print_str(bg_hex);
        bios_putchar(7);
    }
}

static inline void tui_set_attr(bool bold, bool inverse) {
    if (bold) {
        print_str("\x1b[1m");
    }
    if (inverse) {
        print_str("\x1b[7m");
    }
    if (!bold && !inverse) {
        print_str("\x1b[0m");
    }
}

static inline void tui_reset_color(void) { print_str("\x1b[0m"); }

static inline void tui_fill_rect(uint8_t row, uint8_t col, uint8_t width, uint8_t height, char ch) {
    for (uint8_t r = 0; r < height; ++r) {
        tui_set_cursor(row + r, col);
        for (uint8_t c = 0; c < width; ++c) {
            bios_putchar(ch);
        }
    }
}

static inline void tui_draw_hline(uint8_t row, uint8_t col, uint8_t len) {
    tui_set_cursor(row, col);
    print_str("\x1b(0");
    for (uint8_t i = 0; i < len; ++i) {
        bios_putchar('q');
    }
    print_str("\x1b(B");
}

static inline void tui_draw_box(uint8_t row, uint8_t col, uint8_t width, uint8_t height) {
    if (width < 2 || height < 2) {
        return;
    }
    tui_set_cursor(row, col);
    print_str("\x1b(0l");
    for (uint8_t i = 0; i < width - 2; ++i) {
        bios_putchar('q');
    }
    bios_putchar('k');

    for (uint8_t r = 1; r < height - 1; ++r) {
        tui_set_cursor(row + r, col);
        bios_putchar('x');
        tui_set_cursor(row + r, col + width - 1);
        bios_putchar('x');
    }

    tui_set_cursor(row + height - 1, col);
    bios_putchar('m');
    for (uint8_t i = 0; i < width - 2; ++i) {
        bios_putchar('q');
    }
    bios_putchar('j');
    print_str("\x1b(B");
}

static inline void tui_draw_shadow(uint8_t row, uint8_t col, uint8_t width, uint8_t height) {
    print_str("\x1b[40m");
    for (uint8_t r = 1; r <= height; ++r) {
        tui_set_cursor(row + r, col + width);
        bios_putchar(' ');
        bios_putchar(' ');
    }
    tui_set_cursor(row + height, col + 2);
    for (uint8_t c = 2; c < width + 2; ++c) {
        bios_putchar(' ');
    }
}

static inline void tui_draw_window(uint8_t row, uint8_t col, uint8_t width, uint8_t height, const char* title) {
    tui_draw_shadow(row, col, width, height);
    tui_set_color(TUI_COLOR_BLACK, TUI_COLOR_WHITE);
    tui_fill_rect(row, col, width, height, ' ');
    tui_draw_box(row, col, width, height);
    if (title != NULL && title[0] != '\0') {
        tui_set_cursor(row, col + 2);
        print_str("\x1b[30;47m ");
        print_str(title);
        bios_putchar(' ');
    }
}

static inline void tui_draw_button(uint8_t row, uint8_t col, const char* label, bool focused) {
    tui_set_cursor(row, col);
    if (focused) {
        print_str("\x1b[37;44;1m< ");
        print_str(label);
        print_str(" >\x1b[0m\x1b[30;47m");
    } else {
        print_str("\x1b[30;47m  ");
        print_str(label);
        print_str("  \x1b[0m\x1b[30;47m");
    }
}

static inline void tui_draw_header(const char* title) {
    tui_set_cursor(1, 1);
    print_str("\x1b[36;44;1m");
    print_str(title);
    tui_set_cursor(2, 1);
    print_str("\x1b[36;44m");
    tui_draw_hline(2, 1, 80);
    print_str("\x1b[0m");
}

static inline uint16_t tui_get_key(void) {
    char c = 0;
    while (c == 0) {
        c = bios_getchar_nb();
    }
    if (c == 27) {
        char next = 0;
        for (volatile int i = 0; i < 2000; ++i) {
            next = bios_getchar_nb();
            if (next != 0) {
                break;
            }
        }
        if (next == '[') {
            char cmd = 0;
            for (volatile int i = 0; i < 2000; ++i) {
                cmd = bios_getchar_nb();
                if (cmd != 0) {
                    break;
                }
            }
            switch (cmd) {
                case 'A':
                    return TUI_KEY_UP;
                case 'B':
                    return TUI_KEY_DOWN;
                case 'C':
                    return TUI_KEY_RIGHT;
                case 'D':
                    return TUI_KEY_LEFT;
                case 'Z':
                    return TUI_KEY_SHIFT_TAB;
                case 'H':
                    return TUI_KEY_HOME;
                case 'F':
                    return TUI_KEY_END;
                case '3': {
                    for (volatile int i = 0; i < 2000; ++i) {
                        if (bios_getchar_nb() == '~') {
                            break;
                        }
                    }
                    return TUI_KEY_DELETE;
                }
                default:
                    return TUI_KEY_ESC;
            }
        }
        return TUI_KEY_ESC;
    }
    if (c == '\r' || c == '\n') {
        return TUI_KEY_ENTER;
    }
    if (c == '\t') {
        return TUI_KEY_TAB;
    }
    return (uint16_t)(uint8_t)c;
}

#endif
