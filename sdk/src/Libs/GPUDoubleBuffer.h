#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum GpuDoubleBufferConstants {
#if defined(MICRODOS) || defined(__MICRODOS__)
    GPU_VRAM_START       = 0x5FC0U,
    GPU_VRAM_END         = 0x7FBFU,
    GPU_BACKBUF_START    = 0x3A00U,
#else
    GPU_VRAM_START       = 0x2000U,
    GPU_VRAM_END         = 0x3FFFU,
    GPU_BACKBUF_START    = 0x6010U,
#endif
    GPU_WIDTH            = 100,
    GPU_STRIDE           = 128,
    GPU_BACKBUF_STRIDE   = 100,
    GPU_HEIGHT           = 64,
};

enum GpuDoubleBufferTiming {
    GPU_ITERATIONS_PER_MS = 19U,
};

typedef int32_t fixed_t;

static inline fixed_t to_fixed(int16_t x) {
    return (fixed_t)((int32_t)x << 8U);
}

static inline int16_t from_fixed(fixed_t x) {
    return (int16_t)(x >> 8U);
}

#define TO_FIXED(x) to_fixed((int16_t)(x))
#define FROM_FIXED(x) from_fixed((fixed_t)(x))

static inline void drawFrame(void) {
    const uint8_t* src = (const uint8_t*)GPU_BACKBUF_START;
    uint8_t* dst = (uint8_t*)GPU_VRAM_START;
    for (uint8_t i = 0; i < (uint8_t)GPU_HEIGHT; ++i) {
        memcpy(dst, src, GPU_WIDTH);
        src += GPU_BACKBUF_STRIDE;
        dst += GPU_STRIDE;
    }
}

static inline void gpu_put_pixel(int8_t x, int8_t y, uint8_t color) {
    if (x < 0 || x >= GPU_WIDTH || y < 0 || y >= GPU_HEIGHT) {
        return;
    }

    uint16_t offset = (uint16_t)((uint16_t)y * (uint16_t)GPU_BACKBUF_STRIDE + (uint8_t)x);
    uint8_t* buf = (uint8_t*)GPU_BACKBUF_START;
    buf[offset] = color;
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
    uint8_t* buf_row = (uint8_t*)(GPU_BACKBUF_START + ((uint16_t)y * (uint16_t)GPU_BACKBUF_STRIDE) + (uint8_t)x);

    for (int8_t j = y; j < max_y; ++j) {
        memset(buf_row, color, width);
        buf_row += GPU_BACKBUF_STRIDE;
    }
}

static inline void gpu_fill_screen(uint8_t color) {
    uint8_t* buf = (uint8_t*)GPU_BACKBUF_START;
    memset(buf, color, (size_t)GPU_BACKBUF_STRIDE * (size_t)GPU_HEIGHT);
}

static inline int16_t gpu_abs(int16_t v) {
    return (v < 0) ? -v : v;
}

static inline void gpu_swap_coord(int8_t* a, int8_t* b) {
    int8_t t = *a;
    *a = *b;
    *b = t;
}

static inline void gpu_swap_coord16(int16_t* a, int16_t* b) {
    int16_t t = *a;
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

static inline void gpu_swap_u8(uint8_t* a, uint8_t* b) {
    uint8_t t = *a;
    *a = *b;
    *b = t;
}

__attribute__((noinline))
static void gpu_draw_tri16(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                          uint8_t color, bool fill) {
    if (!fill) {
        gpu_draw_line((int8_t)x0, (int8_t)y0, (int8_t)x1, (int8_t)y1, color);
        gpu_draw_line((int8_t)x1, (int8_t)y1, (int8_t)x2, (int8_t)y2, color);
        gpu_draw_line((int8_t)x2, (int8_t)y2, (int8_t)x0, (int8_t)y0, color);
        return;
    }

    if (y0 > y1) { gpu_swap_coord16(&x0, &x1); gpu_swap_coord16(&y0, &y1); }
    if (y0 > y2) { gpu_swap_coord16(&x0, &x2); gpu_swap_coord16(&y0, &y2); }
    if (y1 > y2) { gpu_swap_coord16(&x1, &x2); gpu_swap_coord16(&y1, &y2); }

    if (y2 <= 0 || y0 >= GPU_HEIGHT || y0 == y2) return;

    fixed_t dx_long = (fixed_t)(((int32_t)(x2 - x0) << 8U) / (y2 - y0));
    fixed_t x_long = to_fixed(x0);

    for (uint8_t part = 0; part < 2; ++part) {
        int16_t sy = (part == 0) ? y0 : y1;
        int16_t ey = (part == 0) ? y1 : y2;
        int16_t dy = (int16_t)(ey - sy);
        if (dy <= 0) continue;

        int16_t sx = (part == 0) ? x0 : x1;
        int16_t ex = (part == 0) ? x1 : x2;
        fixed_t dx_short = (fixed_t)(((int32_t)(ex - sx) << 8U) / dy);
        fixed_t x_short = to_fixed(sx);

        if (sy < 0) {
            int16_t skip = (int16_t)-sy;
            x_short += (fixed_t)(dx_short * skip);
            if (part == 0) x_long += (fixed_t)(dx_long * skip);
            sy = 0;
        }
        if (ey > GPU_HEIGHT) ey = GPU_HEIGHT;

        for (int16_t y = sy; y < ey; ++y) {
            int16_t xl = from_fixed(x_long);
            int16_t xr = from_fixed(x_short);
            int16_t left = (xl <= xr) ? xl : xr;
            int16_t right = (xl <= xr) ? xr : xl;
            if (left < 0) left = 0;
            if (right >= GPU_WIDTH) right = GPU_WIDTH - 1;
            if (left <= right) {
                uint16_t offset = (uint16_t)((uint16_t)y * (uint16_t)GPU_BACKBUF_STRIDE + (uint16_t)left);
                memset((void*)(GPU_BACKBUF_START + offset), color, (size_t)(right - left + 1));
            }
            x_long += dx_long;
            x_short += dx_short;
        }
    }
}

__attribute__((noinline))
static void gpu_draw_tri_tex16(int16_t x0, int16_t y0, uint8_t u0, uint8_t v0,
                               int16_t x1, int16_t y1, uint8_t u1, uint8_t v1,
                               int16_t x2, int16_t y2, uint8_t u2, uint8_t v2,
                               const uint8_t* tex) {
    if (tex == NULL) return;

    if (y0 > y1) {
        gpu_swap_coord16(&x0, &x1); gpu_swap_coord16(&y0, &y1);
        gpu_swap_u8(&u0, &u1); gpu_swap_u8(&v0, &v1);
    }
    if (y0 > y2) {
        gpu_swap_coord16(&x0, &x2); gpu_swap_coord16(&y0, &y2);
        gpu_swap_u8(&u0, &u2); gpu_swap_u8(&v0, &v2);
    }
    if (y1 > y2) {
        gpu_swap_coord16(&x1, &x2); gpu_swap_coord16(&y1, &y2);
        gpu_swap_u8(&u1, &u2); gpu_swap_u8(&v1, &v2);
    }

    if (y2 <= 0 || y0 >= GPU_HEIGHT || y0 == y2) return;

    int16_t dx1 = (int16_t)(x1 - x0);
    int16_t dy1 = (int16_t)(y1 - y0);
    int16_t dx2 = (int16_t)(x2 - x0);
    int16_t dy2 = (int16_t)(y2 - y0);
    int16_t du1 = (int16_t)(u1 - u0);
    int16_t dv1 = (int16_t)(v1 - v0);
    int16_t du2 = (int16_t)(u2 - u0);
    int16_t dv2 = (int16_t)(v2 - v0);

    int32_t denom = (int32_t)dx1 * dy2 - (int32_t)dx2 * dy1;
    if (denom == 0) return;

    fixed_t dudx = (fixed_t)((((int32_t)du1 * dy2 - (int32_t)du2 * dy1) << 8U) / denom);
    fixed_t dvdx = (fixed_t)((((int32_t)dv1 * dy2 - (int32_t)dv2 * dy1) << 8U) / denom);

    fixed_t dx_long = (fixed_t)(((int32_t)dx2 << 8U) / dy2);
    fixed_t x_long = to_fixed(x0);
    fixed_t u_long = to_fixed((int16_t)u0);
    fixed_t v_long = to_fixed((int16_t)v0);
    fixed_t du_long = (fixed_t)(((int32_t)du2 << 8U) / dy2);
    fixed_t dv_long = (fixed_t)(((int32_t)dv2 << 8U) / dy2);

    for (uint8_t part = 0; part < 2; ++part) {
        int16_t sy = (part == 0) ? y0 : y1;
        int16_t ey = (part == 0) ? y1 : y2;
        int16_t dy = (int16_t)(ey - sy);
        if (dy <= 0) continue;

        int16_t sx = (part == 0) ? x0 : x1;
        int16_t ex = (part == 0) ? x1 : x2;
        fixed_t dx_short = (fixed_t)(((int32_t)(ex - sx) << 8U) / dy);
        fixed_t x_short = to_fixed(sx);

        if (sy < 0) {
            int16_t skip = (int16_t)-sy;
            x_short += (fixed_t)(dx_short * skip);
            if (part == 0) {
                x_long += (fixed_t)(dx_long * skip);
                u_long += (fixed_t)(du_long * skip);
                v_long += (fixed_t)(dv_long * skip);
            }
            sy = 0;
        }
        if (ey > GPU_HEIGHT) ey = GPU_HEIGHT;

        for (int16_t y = sy; y < ey; ++y) {
            int16_t xl = from_fixed(x_long);
            int16_t xr = from_fixed(x_short);
            int16_t left = (xl <= xr) ? xl : xr;
            int16_t right = (xl <= xr) ? xr : xl;
            if (left < 0) left = 0;
            if (right >= GPU_WIDTH) right = GPU_WIDTH - 1;

            if (left <= right) {
                int16_t dx_rel = (int16_t)(left - xl);
                fixed_t u_curr = (fixed_t)(u_long + (fixed_t)((int32_t)dudx * dx_rel));
                fixed_t v_curr = (fixed_t)(v_long + (fixed_t)((int32_t)dvdx * dx_rel));
                uint16_t offset = (uint16_t)((uint16_t)y * (uint16_t)GPU_BACKBUF_STRIDE + (uint16_t)left);
                uint8_t* dst = (uint8_t*)(GPU_BACKBUF_START + offset);
                for (int16_t x = left; x <= right; ++x) {
                    uint8_t u_idx = (uint8_t)(from_fixed(u_curr) & 0x0FU);
                    uint8_t v_idx = (uint8_t)(from_fixed(v_curr) & 0x0FU);
                    *dst++ = tex[(uint8_t)((v_idx << 4U) | u_idx)];
                    u_curr += dudx;
                    v_curr += dvdx;
                }
            }

            x_long += dx_long;
            u_long += du_long;
            v_long += dv_long;
            x_short += dx_short;
        }
    }
}

static inline void gpu_draw_tri8(int8_t x0, int8_t y0, int8_t x1, int8_t y1, int8_t x2, int8_t y2,
                                uint8_t color, bool fill) {
    gpu_draw_tri16((int16_t)x0, (int16_t)y0, (int16_t)x1, (int16_t)y1, (int16_t)x2, (int16_t)y2, color, fill);
}

#ifdef __cplusplus
static inline void gpu_swap_coord(int16_t* a, int16_t* b) {
    gpu_swap_coord16(a, b);
}
static inline void gpu_draw_tri(int8_t x0, int8_t y0, int8_t x1, int8_t y1, int8_t x2, int8_t y2,
                               uint8_t color, bool fill) {
    gpu_draw_tri8(x0, y0, x1, y1, x2, y2, color, fill);
}
static inline void gpu_draw_tri(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                               uint8_t color, bool fill) {
    gpu_draw_tri16(x0, y0, x1, y1, x2, y2, color, fill);
}
#else
#define gpu_draw_tri(x0, y0, x1, y1, x2, y2, color, fill) \
    _Generic((x0), \
        int16_t: gpu_draw_tri16, \
        default: gpu_draw_tri8 \
    )((x0), (y0), (x1), (y1), (x2), (y2), (color), (fill))
#endif

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
    drawFrame();
}
