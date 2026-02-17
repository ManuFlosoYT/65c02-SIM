#ifndef GPU_H
#define GPU_H

#include <string.h>

/* VRAM Constants */
#define GPU_VRAM_START 0x2000
#define GPU_VRAM_END 0x3FFF
#define GPU_WIDTH 100
#define GPU_STRIDE 128
#define GPU_HEIGHT 64

/* Helper Functions */

// Writes a pixel to VRAM if coordinates are within bounds
void gpu_put_pixel(int x, int y, unsigned char color) {
    unsigned int offset;
    unsigned char* vram;

    if (x < 0 || x >= GPU_WIDTH || y < 0 || y >= GPU_HEIGHT) {
        return;
    }
    
    offset = ((unsigned char)y * GPU_STRIDE) + (unsigned char)x;
    vram = (unsigned char*)GPU_VRAM_START;
    vram[offset] = color;
}

// Fills the entire VRAM
void gpu_fill_screen(unsigned char color) {
    unsigned char* vram = (unsigned char*)GPU_VRAM_START;
    memset(vram, color, GPU_STRIDE * GPU_HEIGHT);
}

int gpu_abs(int v) { return (v < 0) ? -v : v; }

void gpu_swap_int(int* a, int* b) {
    int t = *a;
    *a = *b;
    *b = t;
}

// Draws a line using Bresenham's algorithm
void gpu_draw_line(int x0, int y0, int x1, int y1, unsigned char color) {
    int dx = gpu_abs(x1 - x0);
    int dy = -gpu_abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    int e2;

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

// Draws a triangle (Wireframe OR Filled) with sub-pixel rounding
void gpu_draw_tri(int x0, int y0, int x1, int y1, int x2, int y2,
                  unsigned char color, unsigned char fill) {
    int total_height;
    int i;
    int y;
    int second_half;
    int segment_height;
    int A, B;
    int i_segment;
    int temp;
    int x;
    long num;

    if (!fill) {
        gpu_draw_line(x0, y0, x1, y1, color);
        gpu_draw_line(x1, y1, x2, y2, color);
        gpu_draw_line(x2, y2, x0, y0, color);
        return;
    }


    // Sort vertices by Y (y0 <= y1 <= y2)
    if (y0 > y1) { gpu_swap_int(&x0, &x1); gpu_swap_int(&y0, &y1); }
    if (y0 > y2) { gpu_swap_int(&x0, &x2); gpu_swap_int(&y0, &y2); }
    if (y1 > y2) { gpu_swap_int(&x1, &x2); gpu_swap_int(&y1, &y2); }

    if (y2 < 0 || y0 >= GPU_HEIGHT) return;

    total_height = y2 - y0;
    if (total_height == 0) return;

    // Rasterization Loop
    for (i = 0; i < total_height; i++) {
        y = y0 + i;
        
        if (y < 0) continue;
        if (y >= GPU_HEIGHT) break;

        second_half = (i > (y1 - y0) || y1 == y0);
        segment_height = second_half ? (y2 - y1) : (y1 - y0);
        
        if (segment_height == 0) segment_height = 1;
        num = (long)(x2 - x0) * (long)i;
        A = x0 + (int)((num + (total_height / 2)) / total_height);

        if (second_half) {
            i_segment = i - (y1 - y0);
            num = (long)(x2 - x1) * (long)i_segment;
            B = x1 + (int)((num + (segment_height / 2)) / segment_height);
        } else {
            num = (long)(x1 - x0) * (long)i;
            B = x0 + (int)((num + (segment_height / 2)) / segment_height);
        }

        if (A > B) { temp = A; A = B; B = temp; }

        if (A < 0) A = 0;
        if (B >= GPU_WIDTH) B = GPU_WIDTH - 1;
        
        for (x = A; x <= B; x++) {
             gpu_put_pixel(x, y, color);
        }
    }
}

#define GPU_ITERATIONS_PER_MS 19
void GPUdelay(unsigned int ms) {
    volatile unsigned int i;
    volatile unsigned int j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < GPU_ITERATIONS_PER_MS; j++) { }
    }
}

#endif