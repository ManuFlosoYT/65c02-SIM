#include "Include/BIOS.h"
#include "Include/GPU.h"
#include "Include/LCD.h"
#include "Include/SID.h"

/* --- Helper Variables --- */
unsigned int failures = 0;
unsigned int tests_run = 0;

void assert_true(int condition, const char* name) {
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

// Update LCD with current test name
void lcd_status(const char* test_name) {
    lcd_clear();  // Clear
    lcd_print("Running:");
    lcd_instruction(0xC0);  // Second line
    lcd_print(test_name);
}

/* --- Tests --- */

void test_math(void) {
    volatile unsigned char a = 10, b = 20;
    volatile unsigned char c = 21, d = 10;
    volatile unsigned int x = 1000, y = 2000;

    lcd_status("Math Check");

    // 8-bit Math
    assert_true((a + b) == 30, "8-bit Add");
    assert_true((b - a) == 10, "8-bit Sub");
    assert_true((a * b) == 200, "8-bit Mul");
    assert_true((b / a) == 2, "8-bit Div");
    assert_true((c % d) == 1, "8-bit Mod");

    // 16-bit Math
    assert_true((x + y) == 3000, "16-bit Add");
    assert_true((y - x) == 1000, "16-bit Sub");
    assert_true((x * 2) == 2000, "16-bit Mul");
    assert_true((y / 2) == 1000, "16-bit Div");
}

void test_logic(void) {
    int i;
    int sum = 0;
    volatile int t = 1;
    volatile int f = 0;

    lcd_status("Logic Check");

    // Loop
    for (i = 0; i < 10; i++) {
        sum += i;
    }
    assert_true(sum == 45, "Star Sum Loop");  // 0+1+..+9 = 45

    // Conditional
    if (sum == 45) {
        assert_true(1, "If Statement");
    } else {
        assert_true(0, "If Statement");
    }

    // Logical Operators
    assert_true((t && t) == 1, "AND Op");
    assert_true((t || f) == 1, "OR Op");
    assert_true((!f) == 1, "NOT Op");
}

/* Struct & Arrays */
typedef struct {
    int id;
    unsigned char val;
} Item;

void test_structs(void) {
    Item items[3];
    int k;

    lcd_status("Struct Check");

    for (k = 0; k < 3; ++k) {
        items[k].id = k * 100;
        items[k].val = k;
    }

    assert_true(items[2].id == 200, "Struct Array Access");
    assert_true(items[1].val == 1, "Struct Member Access");
}

/* Pointers */
void swap(int* p1, int* p2) {
    int temp = *p1;
    *p1 = *p2;
    *p2 = temp;
}

void test_pointers(void) {
    int v1 = 55;
    int v2 = 99;

    lcd_status("Pointer Check");

    swap(&v1, &v2);

    assert_true(v1 == 99 && v2 == 55, "Pointer Swap");
}

/* LCD Visual Test */
void test_lcd_visual(void) {
    lcd_status("LCD Visual Test");
    print_str("Look at the LCD... It should show 'LCD Visual Test'\n");
    // Small simulated delay with empty loop
    {
        volatile int d;
        for (d = 0; d < 1000; d++);
    }
}

/* Primes (CPU Intensive) */
int is_prime(int n) {
    int i;
    if (n <= 1) return 0;
    for (i = 2; i * i <= n; i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}

void test_primes(void) {
    lcd_status("Prime Calc");
    assert_true(is_prime(7), "Prime 7");
    assert_true(!is_prime(10), "Not Prime 10");
    assert_true(is_prime(13), "Prime 13");
    assert_true(!is_prime(15), "Not Prime 15");
    assert_true(is_prime(101), "Prime 101");  // A bit more costly
}

void test_gpu(void) {
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
}

void test_sound(void) {
    int i;
    // Play C Major Scale
    uint16_t notes[] = {NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4,
                        NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5};

    lcd_status("Sound Test");
    print_str("Test Sound: Playing a scale...\n");

    // Init SID
    sid_set_volume(15);

    // Voice 1 - Pulse
    sid_voice_adsr(1, 0, 9, 0, 0);
    sid_voice_pw(1, 0x800);

    for (i = 0; i < 8; ++i) {
        sid_voice_freq(1, notes[i]);
        sid_voice_control(1, WAVE_PULSE | WAVE_GATE);
        sid_delay(3000);
        sid_voice_control(1, WAVE_PULSE);  // Release
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

    // Final Report
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
