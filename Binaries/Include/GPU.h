#ifndef GPU_H
#define GPU_H

#include <string.h>

/* VRAM Constants */
#define GPU_VRAM_START 0x2000
#define GPU_VRAM_END 0x3FFF
#define GPU_WIDTH 100
#define GPU_STRIDE 128
#define GPU_HEIGHT 64

// Writes a pixel to VRAM if coordinates are within bounds
void gpu_put_pixel(signed char x, signed char y, unsigned char color) {
    unsigned short offset;
    unsigned char* vram;

    if (x < 0 || x >= GPU_WIDTH || y < 0 || y >= GPU_HEIGHT) {
        return;
    }

    // Y * 128 + X
    offset = ((unsigned short)y << 7) + (unsigned char)x;
    vram = (unsigned char*)(unsigned long)GPU_VRAM_START;
    vram[offset] = color;
}

void gpu_draw_rect( signed char x, signed char y,
                    signed char w, signed char h,
                   unsigned char color) {
    signed char i, j;
    signed char max_x, max_y;
    if (x >= GPU_WIDTH || y >= GPU_HEIGHT) return;
    if (x + w < 0 || y + h < 0) return;

    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }

    max_x = x + w;
    max_y = y + h;

    if (max_x > GPU_WIDTH) max_x = GPU_WIDTH;
    if (max_y > GPU_HEIGHT) max_y = GPU_HEIGHT;

    for (j = y; j < max_y; j++) {
        unsigned char* vram_row =
            (unsigned char*)(unsigned long)(GPU_VRAM_START +
                                            ((unsigned short)j << 7));
        for (i = x; i < max_x; i++) {
            vram_row[i] = color;
        }
    }
}

// Fills the entire VRAM
void gpu_fill_screen(unsigned char color) {
    unsigned char* vram = (unsigned char*)GPU_VRAM_START;
    memset(vram, color, GPU_STRIDE * GPU_HEIGHT);
}

int gpu_abs(int v) { return (v < 0) ? -v : v; }

void gpu_swap_coord(signed char* a, signed char* b) {
    signed char t = *a;
    *a = *b;
    *b = t;
}

// Draws a line using Bresenham's algorithm
void gpu_draw_line( signed char x0, signed char y0, 
                    signed char x1, signed char y1, 
                    unsigned char color) {
                        
    signed char dx = (signed char)gpu_abs(x1 - x0);
    signed char dy = (signed char)-gpu_abs(y1 - y0);
    signed char sx = (x0 < x1) ? 1 : -1;
    signed char sy = (y0 < y1) ? 1 : -1;
    short err = dx + dy;
    short e2;

    while (1) {
        gpu_put_pixel(x0, y0, color);

        if (x0 == x1 && y0 == y1) break;

        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

typedef short fixed_t;
#define TO_FIXED(x) ((x) << 8)
#define FROM_FIXED(x) ((x) >> 8)

// Draws a triangle (Wireframe OR Filled)
void gpu_draw_tri(signed char x0, signed char y0, signed char x1,
                  signed char y1, signed char x2, signed char y2,
                  unsigned char color, unsigned char fill) {
    fixed_t x_long, x_short;
    fixed_t dx_long, dx_short;
    signed char y, start_y, end_y;
    short xl, xr, t;
    unsigned short offset;

    if (!fill) {
        gpu_draw_line(x0, y0, x1, y1, color);
        gpu_draw_line(x1, y1, x2, y2, color);
        gpu_draw_line(x2, y2, x0, y0, color);
        return;
    }

    // Sort vertices by Y (y0 <= y1 <= y2)
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

    if (y2 < 0 || y0 >= GPU_HEIGHT) return;

    x_long = TO_FIXED(x0);
    dx_long = 0;

    if (y2 != y0) dx_long = TO_FIXED(x2 - x0) / (y2 - y0);

    if (y1 > y0) {
        x_short = TO_FIXED(x0);
        dx_short = TO_FIXED(x1 - x0) / (y1 - y0);

        start_y = y0;
        end_y = y1;

        if (start_y < 0) {
            x_long += dx_long * (-start_y);
            x_short += dx_short * (-start_y);
            start_y = 0;
        }

        if (end_y > GPU_HEIGHT) end_y = GPU_HEIGHT;

        for (y = start_y; y < end_y; y++) {
            xl = FROM_FIXED(x_long);
            xr = FROM_FIXED(x_short);
            if (xl > xr) {
                t = xl;
                xl = xr;
                xr = t;
            }

            if (xl < 0) xl = 0;
            if (xr >= GPU_WIDTH) xr = GPU_WIDTH - 1;

            if (xl <= xr) {
                offset = ((unsigned short)y << 7) + xl;
                memset((void*)(unsigned long)(GPU_VRAM_START + offset), color,
                       xr - xl + 1);
            }

            x_long += dx_long;
            x_short += dx_short;
        }
    } else {
        if (y0 < 0) {
            x_long += dx_long * (-y0);
        }
    }

    if (y2 > y1) {
        x_short = TO_FIXED(x1);
        dx_short = TO_FIXED(x2 - x1) / (y2 - y1);

        start_y = y1;
        end_y = y2;

        if (start_y < 0) {
            x_long += dx_long * (0 - start_y);
            x_short += dx_short * (0 - start_y);
            start_y = 0;
        }
        if (y0 == y1 && y0 < 0) {
            x_long += dx_long * (-y0);
        }

        if (end_y > GPU_HEIGHT) end_y = GPU_HEIGHT;

        for (y = start_y; y < end_y; y++) {
            xl = FROM_FIXED(x_long);
            xr = FROM_FIXED(x_short);
            if (xl > xr) {
                t = xl;
                xl = xr;
                xr = t;
            }

            if (xl < 0) xl = 0;
            if (xr >= GPU_WIDTH) xr = GPU_WIDTH - 1;

            if (xl <= xr) {
                offset = ((unsigned short)y << 7) + xl;
                memset((void*)(unsigned long)(GPU_VRAM_START + offset), color,
                       xr - xl + 1);
            }

            x_long += dx_long;
            x_short += dx_short;
        }
    }
}

#define GPU_ITERATIONS_PER_MS 19
void GPUdelay(unsigned int ms) {
    volatile unsigned short i;
    volatile unsigned short j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < GPU_ITERATIONS_PER_MS; j++) {
        }
    }
}

void gpu_test_pattern() {
    unsigned char r, c;
    unsigned char row_color;
    for (r = 0; r < GPU_HEIGHT; r++) {
        row_color = r * 4;
        for (c = 0; c < GPU_WIDTH; c++) {
            gpu_put_pixel(c, r, row_color + c);
        }
    }
}

#endif