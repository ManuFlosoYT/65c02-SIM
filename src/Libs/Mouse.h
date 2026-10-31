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

#endif
