#include "Libs/BIOS.h"
#include "Libs/GPU.h"

int main() {
    INIT_BUFFER();
    print_str("Starting GPU Test...\n");

    // 1. Fill screen with a background color
    print_str("Test 1: Fill Screen (Color 0x03)\n");
    gpu_fill_screen(0x03);
    print_str("Done. Waiting...\n");
    delay(10);

    // 2. Draw some rectangles
    print_str("Test 2: Draw Rectangles\n");
    gpu_draw_rect(5, 5, 20, 10, 0xE0);    // Red-ish
    gpu_draw_rect(30, 10, 15, 15, 0x1C);  // Green-ish
    gpu_draw_rect(50, 5, 40, 5, 0x3C);    // Blue-ish
    print_str("Done. Waiting...\n");
    delay(10);

    // 3. Draw lines
    print_str("Test 3: Draw Lines\n");
    gpu_draw_line(0, 0, 99, 63, 0xFF);  // Diagonal White
    gpu_draw_line(99, 0, 0, 63, 0xFF);  // Diagonal White
    print_str("Done. Waiting...\n");
    delay(10);

    // 4. Draw Triangles
    print_str("Test 4: Draw Triangles\n");
    // Wireframe
    gpu_draw_tri(10, 40, 20, 20, 30, 40, 0xE0, 0);
    // Filled
    gpu_draw_tri(50, 20, 35, 45, 65, 45, 0x1C, 1);
    print_str("Done. Waiting...\n");
    delay(30);

    // 5. Complex Test Pattern
    print_str("Test 5: Built-in Pattern\n");
    gpu_fill_screen(0x00);
    gpu_test_pattern();

    print_str("GPU Test Complete.\n");
    return 0;
}
