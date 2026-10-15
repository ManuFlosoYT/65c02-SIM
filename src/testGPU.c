#include "Libs/BIOS.h"
#include "Libs/GPU.h"

int main(void) {
    INIT_BUFFER();
    print_str("Starting GPU Test...\n");

    print_str("Test 1: Fill Screen (Color 0x03)\n");
    gpu_fill_screen(0x03U);
    print_str("Done. Waiting...\n");
    delay(10);

    print_str("Test 2: Draw Rectangles\n");
    gpu_draw_rect(5, 5, 20, 10, 0xE0U);
    gpu_draw_rect(30, 10, 15, 15, 0x1CU);
    gpu_draw_rect(50, 5, 40, 5, 0x3CU);
    print_str("Done. Waiting...\n");
    delay(10);

    print_str("Test 3: Draw Lines\n");
    gpu_draw_line(0, 0, 99, 63, 0xFFU);
    gpu_draw_line(99, 0, 0, 63, 0xFFU);
    print_str("Done. Waiting...\n");
    delay(10);

    print_str("Test 4: Draw Triangles\n");
    gpu_draw_tri(10, 40, 20, 20, 30, 40, 0xE0U, false);
    gpu_draw_tri(50, 20, 35, 45, 65, 45, 0x1CU, true);
    print_str("Done. Waiting...\n");
    delay(30);

    print_str("Test 5: Built-in Pattern\n");
    gpu_fill_screen(0x00U);
    gpu_test_pattern();

    print_str("GPU Test Complete.\n");
    return 0;
}
