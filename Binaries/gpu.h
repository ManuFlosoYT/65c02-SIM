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
void gpu_put_pixel(unsigned char x, unsigned char y, unsigned char color) {
    unsigned int offset;
    unsigned char* vram;

    if (x >= GPU_WIDTH || y >= GPU_HEIGHT) {
        return;
    }
    offset = (y * GPU_STRIDE) + x;
    vram = (unsigned char*)GPU_VRAM_START;
    vram[offset] = color;
}

// Fills the entire VRAM (including blanking/stride) with a color
void gpu_fill_screen(unsigned char color) {
    unsigned char* vram = (unsigned char*)GPU_VRAM_START;
    memset(vram, color, GPU_STRIDE * GPU_HEIGHT);
}

// Draws a filled rectangle
void gpu_draw_rect(unsigned char x, unsigned char y, unsigned char w,
                   unsigned char h, unsigned char color) {
    unsigned char i, j;
    unsigned char max_x;
    unsigned char max_y;

    // Clip to screen
    if (x >= GPU_WIDTH || y >= GPU_HEIGHT) return;

    max_x = x + w;
    max_y = y + h;

    if (max_x > GPU_WIDTH) max_x = GPU_WIDTH;
    if (max_y > GPU_HEIGHT) max_y = GPU_HEIGHT;

    for (j = y; j < max_y; j++) {
        for (i = x; i < max_x; i++) {
            gpu_put_pixel(i, j, color);
        }
    }
}

int gpu_abs(int v) { return (v < 0) ? -v : v; }

void gpu_swap(unsigned char* a, unsigned char* b) {
    unsigned char t = *a;
    *a = *b;
    *b = t;
}

// Draws a line using Bresenham's algorithm
void gpu_draw_line(unsigned char x0, unsigned char y0, 
                   unsigned char x1, unsigned char y1, 
                   unsigned char color) {
    int dx = gpu_abs((int)x1 - (int)x0);
    int dy = -gpu_abs((int)y1 - (int)y0);
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

// Internal helper for filled triangle
void _gpu_draw_scanline(int y, int x1, int x2, unsigned char color) {
    int x;
    int temp;
    if (x1 > x2) {
        temp = x1;
        x1 = x2;
        x2 = temp;
    }
    for (x = x1; x <= x2; x++) {
        gpu_put_pixel((unsigned char)x, (unsigned char)y, color);
    }
}

// Draws a triangle (wireframe or filled)
// Fill logic: Standard scanline rasterization using integer arithmetic
void gpu_draw_tri(unsigned char x0, unsigned char y0, 
                  unsigned char x1, unsigned char y1, 
                  unsigned char x2, unsigned char y2,
                  unsigned char color, unsigned char fill) {
    if (!fill) {
        gpu_draw_line(x0, y0, x1, y1, color);
        gpu_draw_line(x1, y1, x2, y2, color);
        gpu_draw_line(x2, y2, x0, y0, color);
    } else {
        int total_height;
        int i;
        int y;
        int second_half;
        int segment_height;
        int A, B;
        int i_segment;
        int temp;
        int x;

        // Sort vertices by y (y0 <= y1 <= y2)
        if (y0 > y1) {
            gpu_swap(&x0, &x1);
            gpu_swap(&y0, &y1);
        }
        if (y0 > y2) {
            gpu_swap(&x0, &x2);
            gpu_swap(&y0, &y2);
        }
        if (y1 > y2) {
            gpu_swap(&x1, &x2);
            gpu_swap(&y1, &y2);
        }

        total_height = y2 - y0;
        if (total_height == 0) return;  // Degenerate triangle

        for (i = 0; i < total_height; i++) {
            y = y0 + i;
            second_half = (i > (y1 - y0) || y1 == y0);
            segment_height = second_half ? (y2 - y1) : (y1 - y0);

            // Avoid division by zero
            if (segment_height == 0) segment_height = 1;

            // Integer arithmetic for interpolation
            // A = x0 + (x2 - x0) * i / total_height
            A = x0 + ((int)(x2 - x0) * (int)i) / total_height;

            // B depends on which half of the triangle we are in
            if (second_half) {
                i_segment = i - (y1 - y0);
                B = x1 + ((int)(x2 - x1) * i_segment) / segment_height;
            } else {
                B = x0 + ((int)(x1 - x0) * (int)i) / segment_height;
            }

            // Fill the scanline
            if (A > B) {
                temp = A;
                A = B;
                B = temp;
            }
            for (x = A; x <= B; x++) {
                // Clip x to screen width
                if (x >= 0 && x < GPU_WIDTH) {
                    gpu_put_pixel((unsigned char)x, (unsigned char)y, color);
                }
            }
        }
    }
}

// Generates a test pattern similar to colortest.s
void gpu_test_pattern() {
    unsigned char r, c;
    unsigned char row_color;
    for (r = 0; r < GPU_HEIGHT; r++) {
        // Create a gradient or pattern
        row_color = r * 4;
        for (c = 0; c < GPU_WIDTH; c++) {
            gpu_put_pixel(c, r, row_color + c);
        }
    }
}

/* Función: delay
 * Descripción: Genera un retardo en milisegundos.
 * Parámetros: ms (cantidad de milisegundos a esperar)
 */
#define GPU_ITERACIONES_POR_MS 19
void GPUdelay(unsigned int ms) {
    volatile unsigned int i;
    volatile unsigned int j;

    for (i = 0; i < ms; i++) {
        /* Este bucle interno consume aprox 1 milisegundo */
        for (j = 0; j < GPU_ITERACIONES_POR_MS; j++) {
            /* Bucle vacío para gastar ciclos */
        }
    }
}

#endif
