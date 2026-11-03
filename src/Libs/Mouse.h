#pragma once
#ifndef LIBS_MOUSE_H
#define LIBS_MOUSE_H

#include <stdbool.h>
#include <stdint.h>

#if defined(MICRODOS) || defined(__MICRODOS__)
#ifndef GPU_VRAM_START
#define GPU_VRAM_START 0x5FC0U
#define _MOUSE_TEMP_GPU_VRAM
#endif
#else
#ifndef GPU_VRAM_START
#define GPU_VRAM_START 0x2000U
#define _MOUSE_TEMP_GPU_VRAM
#endif
#endif

enum MouseConstants {
    MOUSE_REG_BTNS_ADDR = (GPU_VRAM_START + 0x1FF0U),
    MOUSE_REG_X_ADDR    = (GPU_VRAM_START + 0x1FF1U),
    MOUSE_REG_Y_ADDR    = (GPU_VRAM_START + 0x1FF2U),

    MOUSE_BTN_LEFT   = 0x01U,
    MOUSE_BTN_RIGHT  = 0x02U,
    MOUSE_BTN_MIDDLE = 0x04U,

    MOUSE_CURSOR_WIDTH  = 6,
    MOUSE_CURSOR_HEIGHT = 8,
};

#ifdef _MOUSE_TEMP_GPU_VRAM
#undef GPU_VRAM_START
#undef _MOUSE_TEMP_GPU_VRAM
#endif

#define MOUSE_BTNS (*(volatile uint8_t*)MOUSE_REG_BTNS_ADDR)
#define MOUSE_X    (*(volatile uint8_t*)MOUSE_REG_X_ADDR)
#define MOUSE_Y    (*(volatile uint8_t*)MOUSE_REG_Y_ADDR)

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t buttons;
} MouseState;

static inline uint8_t mouse_get_x(void) {
    return MOUSE_X;
}

static inline uint8_t mouse_get_y(void) {
    return MOUSE_Y;
}

static inline uint8_t mouse_get_buttons(void) {
    return MOUSE_BTNS;
}

static inline bool mouse_is_btn_pressed(uint8_t btn_mask) {
    return (MOUSE_BTNS & btn_mask) != 0U;
}

static inline bool mouse_btn_left(void) {
    return (MOUSE_BTNS & MOUSE_BTN_LEFT) != 0U;
}

static inline bool mouse_btn_right(void) {
    return (MOUSE_BTNS & MOUSE_BTN_RIGHT) != 0U;
}

static inline bool mouse_btn_middle(void) {
    return (MOUSE_BTNS & MOUSE_BTN_MIDDLE) != 0U;
}

static inline void mouse_read(MouseState* state) {
    if (state != 0) {
        state->x = MOUSE_X;
        state->y = MOUSE_Y;
        state->buttons = MOUSE_BTNS;
    }
}

static inline bool mouse_in_rect(uint8_t rx, uint8_t ry, uint8_t rw, uint8_t rh) {
    uint8_t mx = MOUSE_X;
    uint8_t my = MOUSE_Y;
    return (mx >= rx && mx < (uint8_t)(rx + rw) && my >= ry && my < (uint8_t)(ry + rh));
}

static inline void mouse_draw_cursor_at(uint8_t x, uint8_t y) {
    static const uint8_t MOUSE_CURSOR_MASK[8]  = {0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xF8, 0xD8, 0x0C};
    static const uint8_t MOUSE_CURSOR_COLOR[8] = {0x00, 0x00, 0x40, 0x60, 0x70, 0x60, 0x00, 0x00};
#if defined(MICRODOS) || defined(__MICRODOS__)
    uint8_t* vram = (uint8_t*)0x5FC0U;
#else
    uint8_t* vram = (uint8_t*)0x2000U;
#endif

    for (uint8_t r = 0; r < MOUSE_CURSOR_HEIGHT; ++r) {
        int8_t py = (int8_t)(y + r);
        if (py < 0 || py >= 64) {
            continue;
        }
        uint8_t m = MOUSE_CURSOR_MASK[r];
        uint8_t c = MOUSE_CURSOR_COLOR[r];
        uint16_t row_offset = (uint16_t)py << 7U;
        for (uint8_t col = 0; col < MOUSE_CURSOR_WIDTH; ++col) {
            uint8_t bit = (uint8_t)(0x80U >> col);
            if (m & bit) {
                int8_t px = (int8_t)(x + col);
                if (px >= 0 && px < 100) {
                    vram[row_offset + (uint8_t)px] = (c & bit) ? 0x3FU : 0x00U;
                }
            }
        }
    }
}

#ifndef __cplusplus
#define _MOUSE_DRAW_GET_MACRO(_0, _1, _2, NAME, ...) NAME
#define mouse_draw_cursor(...) _MOUSE_DRAW_GET_MACRO(dummy, ##__VA_ARGS__, mouse_draw_cursor_at, _mouse_draw_cursor_0)(__VA_ARGS__)
static inline void _mouse_draw_cursor_0(void) {
    mouse_draw_cursor_at(MOUSE_X, MOUSE_Y);
}
#else
static inline void mouse_draw_cursor(uint8_t x, uint8_t y) {
    mouse_draw_cursor_at(x, y);
}
static inline void mouse_draw_cursor(void) {
    mouse_draw_cursor_at(MOUSE_X, MOUSE_Y);
}
#endif

#endif
