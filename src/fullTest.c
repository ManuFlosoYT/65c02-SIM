#include <stdbool.h>
#include <stdint.h>

#include "Libs/BIOS.h"
#include "Libs/GPU.h"
#include "Libs/LCD.h"
#include "Libs/SID.h"

static uint16_t failures = 0;
static uint16_t tests_run = 0;

void assert_true(bool condition, const char* name) {
    tests_run++;
    print_str("Test: ");
    print_str(name);
    print_str(" ... ");
    if (condition) {
        print_str("PASS\n");
    } else {
        print_str("FAIL\n");
        failures++;
    }
}

void lcd_status(const char* test_name) {
    lcd_clear();
    lcd_print("Running:");
    lcd_instruction(0xC0);
    lcd_print(test_name);
}

void test_math(void) {
    volatile uint8_t a = 10;
    volatile uint8_t b = 20;
    volatile uint8_t c = 21;
    volatile uint8_t d = 10;
    volatile uint16_t x = 1000;
    volatile uint16_t y = 2000;

    lcd_status("Math Check");

    assert_true((a + b) == 30, "8-bit Add");
    assert_true((b - a) == 10, "8-bit Sub");
    assert_true((a * b) == 200, "8-bit Mul");
    assert_true((b / a) == 2, "8-bit Div");
    assert_true((c % d) == 1, "8-bit Mod");

    assert_true((x + y) == 3000, "16-bit Add");
    assert_true((y - x) == 1000, "16-bit Sub");
    assert_true((x * 2) == 2000, "16-bit Mul");
    assert_true((y / 2) == 1000, "16-bit Div");
}

void test_logic(void) {
    int sum = 0;
    volatile bool t = true;
    volatile bool f = false;

    lcd_status("Logic Check");

    for (int i = 0; i < 10; i++) {
        sum += i;
    }
    assert_true(sum == 45, "Star Sum Loop");

    if (sum == 45) {
        assert_true(true, "If Statement");
    } else {
        assert_true(false, "If Statement");
    }

    assert_true(t && t, "AND Op");
    assert_true(t || f, "OR Op");
    assert_true(!f, "NOT Op");
}

typedef struct {
    int16_t id;
    uint8_t val;
} Item;

void test_structs(void) {
    Item items[3];

    lcd_status("Struct Check");

    for (int16_t k = 0; k < 3; ++k) {
        items[k].id = (int16_t)(k * 100);
        items[k].val = (uint8_t)k;
    }

    assert_true(items[2].id == 200, "Struct Array Access");
    assert_true(items[1].val == 1, "Struct Member Access");
}

void swap(int16_t* p1, int16_t* p2) {
    int16_t temp = *p1;
    *p1 = *p2;
    *p2 = temp;
}

void test_pointers(void) {
    int16_t v1 = 55;
    int16_t v2 = 99;

    lcd_status("Pointer Check");

    swap(&v1, &v2);

    assert_true(v1 == 99 && v2 == 55, "Pointer Swap");
}

void test_lcd_visual(void) {
    lcd_status("LCD Visual Test");
    print_str("Look at the LCD... It should show 'LCD Visual Test'\n");
    for (volatile uint16_t d = 0; d < 1000; ++d) {
    }
}

bool is_prime(int16_t n) {
    if (n <= 1) {
        return false;
    }
    for (int16_t i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

void test_primes(void) {
    lcd_status("Prime Calc");
    assert_true(is_prime(7), "Prime 7");
    assert_true(!is_prime(10), "Not Prime 10");
    assert_true(is_prime(13), "Prime 13");
    assert_true(!is_prime(15), "Not Prime 15");
    assert_true(is_prime(101), "Prime 101");
}

void test_gpu(void) {
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
}

void test_sound(void) {
    const uint16_t notes[] = {
        NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4,
        NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5,
    };

    lcd_status("Sound Test");
    print_str("Test Sound: Playing a scale...\n");

    sid_set_volume(15);

    sid_voice_adsr(1, 0, 9, 0, 0);
    sid_voice_pw(1, 0x800U);

    for (uint8_t i = 0; i < 8; ++i) {
        sid_voice_freq(1, notes[i]);
        sid_voice_control(1, (uint8_t)((unsigned int)WAVE_PULSE | (unsigned int)WAVE_GATE));
        sid_delay(3000);
        sid_voice_control(1, WAVE_PULSE);
        sid_delay(500);
    }

    print_str("Sound Test Complete.\n");
}

int main(void) {
    INIT_BUFFER();
    lcd_init();

    print_str("\n=== START FULLTEST ===\n");
    lcd_print("Starting...");

    test_math();
    test_logic();
    test_structs();
    test_pointers();
    test_primes();
    test_lcd_visual();
    test_gpu();
    test_sound();

    lcd_clear();
    lcd_print("Tests completed");

    print_str("\n=== RESULTS ===\n");
    print_str("Total Tests: ");
    print_num(tests_run);
    print_str("\n");
    print_str("Failures: ");
    print_num(failures);
    print_str("\n");

    if (failures == 0) {
        print_str("STATUS: OK\n");
        lcd_instruction(0xC0);
        lcd_print("Status: OK");
    } else {
        print_str("STATUS: FAILED\n");
        lcd_instruction(0xC0);
        lcd_print("Status: FAIL");
    }

    return 0;
}
