#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum GpuConstants {
#if defined(MICRODOS) || defined(__MICRODOS__)
    GPU_VRAM_START = 0x5FC0U,
    GPU_VRAM_END   = 0x7FBFU,
#else
    GPU_VRAM_START = 0x2000U,
    GPU_VRAM_END   = 0x3FFFU,
#endif
    GPU_WIDTH      = 100,
    GPU_STRIDE     = 128,
    GPU_HEIGHT     = 64,
};

enum GpuTiming {
    GPU_ITERATIONS_PER_MS = 19U,
};

typedef int16_t fixed_t;

static inline fixed_t to_fixed(int16_t x) {
    return (fixed_t)(x << 8U);
}

static inline int16_t from_fixed(fixed_t x) {
    return (int16_t)(x >> 8U);
}

#define TO_FIXED(x) to_fixed((int16_t)(x))
#define FROM_FIXED(x) from_fixed((fixed_t)(x))

static inline void gpu_put_pixel(int8_t x, int8_t y, uint8_t color) {
    if (x < 0 || x >= GPU_WIDTH || y < 0 || y >= GPU_HEIGHT) {
        return;
    }

    uint16_t offset = ((uint16_t)y << 7U) + (uint8_t)x;
    uint8_t* vram = (uint8_t*)GPU_VRAM_START;
    vram[offset] = color;
}

static inline void gpu_draw_rect(int8_t x, int8_t y, int8_t w, int8_t h, uint8_t color) {
    if (w <= 0 || h <= 0 || x >= GPU_WIDTH || y >= GPU_HEIGHT) {
        return;
    }
    if ((int16_t)x + (int16_t)w <= 0 || (int16_t)y + (int16_t)h <= 0) {
        return;
    }

    if (x < 0) {
        w = (int8_t)(w + x);
        x = 0;
    }
    if (y < 0) {
        h = (int8_t)(h + y);
        y = 0;
    }

    int8_t max_x = (int8_t)(x + w);
    int8_t max_y = (int8_t)(y + h);

    if (max_x > GPU_WIDTH) {
        max_x = GPU_WIDTH;
    }
    if (max_y > GPU_HEIGHT) {
        max_y = GPU_HEIGHT;
    }

    if (max_x <= x || max_y <= y) {
        return;
    }

    size_t width = (size_t)(max_x - x);
    uint8_t* vram_row = (uint8_t*)(GPU_VRAM_START + ((uint16_t)y << 7U) + (uint8_t)x);

    for (int8_t j = y; j < max_y; ++j) {
        memset(vram_row, color, width);
        vram_row += GPU_STRIDE;
    }
}

static inline void gpu_fill_screen(uint8_t color) {
    uint8_t* vram = (uint8_t*)GPU_VRAM_START;
    memset(vram, color, (size_t)GPU_STRIDE * (size_t)GPU_HEIGHT);
}

static inline int16_t gpu_abs(int16_t v) {
    return (v < 0) ? -v : v;
}

static inline void gpu_swap_coord(int8_t* a, int8_t* b) {
    int8_t t = *a;
    *a = *b;
    *b = t;
}

static inline void gpu_draw_line(int8_t x0, int8_t y0, int8_t x1, int8_t y1, uint8_t color) {
    int8_t dx = (int8_t)gpu_abs(x1 - x0);
    int8_t dy = (int8_t)-gpu_abs(y1 - y0);
    int8_t sx = (x0 < x1) ? 1 : -1;
    int8_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = (int16_t)(dx + dy);

    while (true) {
        gpu_put_pixel(x0, y0, color);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        int16_t e2 = (int16_t)(2 * err);
        if (e2 >= dy) {
            err = (int16_t)(err + dy);
            x0 = (int8_t)(x0 + sx);
        }
        if (e2 <= dx) {
            err = (int16_t)(err + dx);
            y0 = (int8_t)(y0 + sy);
        }
    }
}

static inline void gpu_draw_tri(int8_t x0, int8_t y0, int8_t x1, int8_t y1, int8_t x2, int8_t y2,
                               uint8_t color, bool fill) {
    if (!fill) {
        gpu_draw_line(x0, y0, x1, y1, color);
        gpu_draw_line(x1, y1, x2, y2, color);
        gpu_draw_line(x2, y2, x0, y0, color);
        return;
    }

    if (y0 > y1) {
        gpu_swap_coord(&x0, &x1);
        gpu_swap_coord(&y0, &y1);
    }
    if (y0 > y2) {
        gpu_swap_coord(&x0, &x2);
        gpu_swap_coord(&y0, &y2);
    }
    if (y1 > y2) {
        gpu_swap_coord(&x1, &x2);
        gpu_swap_coord(&y1, &y2);
    }

    if (y2 < 0 || y0 >= GPU_HEIGHT) {
        return;
    }

    fixed_t x_long = to_fixed(x0);
    fixed_t dx_long = 0;

    if (y2 != y0) {
        dx_long = (fixed_t)(to_fixed((int16_t)(x2 - x0)) / (y2 - y0));
    }

    if (y1 > y0) {
        fixed_t x_short = to_fixed(x0);
        fixed_t dx_short = (fixed_t)(to_fixed((int16_t)(x1 - x0)) / (y1 - y0));

        int8_t start_y = y0;
        int8_t end_y = y1;

        if (start_y < 0) {
            x_long += (fixed_t)(dx_long * -start_y);
            x_short += (fixed_t)(dx_short * -start_y);
            start_y = 0;
        }

        if (end_y > GPU_HEIGHT) {
            end_y = GPU_HEIGHT;
        }

        for (int8_t y = start_y; y < end_y; ++y) {
            int16_t xl = from_fixed(x_long);
            int16_t xr = from_fixed(x_short);
            if (xl > xr) {
                int16_t t = xl;
                xl = xr;
                xr = t;
            }

            if (xl < 0) {
                xl = 0;
            }
            if (xr >= GPU_WIDTH) {
                xr = GPU_WIDTH - 1;
            }

            if (xl <= xr) {
                uint16_t offset = ((uint16_t)y << 7U) + (uint16_t)xl;
                memset((void*)(GPU_VRAM_START + offset), color, (size_t)(xr - xl + 1));
            }

            x_long += dx_long;
            x_short += dx_short;
        }
    } else {
        if (y0 < 0) {
            x_long += (fixed_t)(dx_long * -y0);
        }
    }

    if (y2 > y1) {
        fixed_t x_short = to_fixed(x1);
        fixed_t dx_short = (fixed_t)(to_fixed((int16_t)(x2 - x1)) / (y2 - y1));

        int8_t start_y = y1;
        int8_t end_y = y2;

        if (start_y < 0) {
            x_long += (fixed_t)(dx_long * -start_y);
            x_short += (fixed_t)(dx_short * -start_y);
            start_y = 0;
        }
        if (y0 == y1 && y0 < 0) {
            x_long += (fixed_t)(dx_long * -y0);
        }

        if (end_y > GPU_HEIGHT) {
            end_y = GPU_HEIGHT;
        }

        for (int8_t y = start_y; y < end_y; ++y) {
            int16_t xl = from_fixed(x_long);
            int16_t xr = from_fixed(x_short);
            if (xl > xr) {
                int16_t t = xl;
                xl = xr;
                xr = t;
            }

            if (xl < 0) {
                xl = 0;
            }
            if (xr >= GPU_WIDTH) {
                xr = GPU_WIDTH - 1;
            }

            if (xl <= xr) {
                uint16_t offset = ((uint16_t)y << 7U) + (uint16_t)xl;
                memset((void*)(GPU_VRAM_START + offset), color, (size_t)(xr - xl + 1));
            }

            x_long += dx_long;
            x_short += dx_short;
        }
    }
}

static inline void GPUdelay(uint16_t ms) {
    for (uint16_t i = 0; i < ms; ++i) {
        for (volatile uint16_t j = 0; j < GPU_ITERATIONS_PER_MS; ++j) {
        }
    }
}

static inline void gpu_test_pattern(void) {
    for (uint8_t r = 0; r < (uint8_t)GPU_HEIGHT; ++r) {
        uint8_t row_color = (uint8_t)(r * 4U);
        for (uint8_t c = 0; c < (uint8_t)GPU_WIDTH; ++c) {
            gpu_put_pixel((int8_t)c, (int8_t)r, (uint8_t)(row_color + c));
        }
    }
}