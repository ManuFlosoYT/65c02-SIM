#include "bios.h"
#include "lcd.h"

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

// Actualizamos el LCD con el nombre del test actual
void lcd_status(const char* test_name) {
    lcd_instruccion(0x01);  // Limpiar
    lcd_imprimir("Ejecutando:");
    lcd_instruccion(0xC0);  // Segunda linea
    lcd_imprimir(test_name);
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
    print_str("Mira el LCD... Deberia mostrar 'LCD Visual Test'\n");
    // Pequeño retardo simulado con loop vacio
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
    assert_true(is_prime(101), "Prime 101");  // Poco mas costoso
}

int main(void) {
    INIT_BUFFER();
    lcd_inicializar();

    print_str("\n=== INICIO FULLTEST ===\n");
    lcd_imprimir("Iniciando...");

    test_math();
    test_logic();
    test_structs();
    test_pointers();
    test_primes();
    test_lcd_visual();

    // Final Report
    lcd_instruccion(0x01);
    lcd_imprimir("Tests completados");

    print_str("\n=== RESULTADOS ===\n");
    print_str("Total Tests: ");
    print_num(tests_run);
    print_str("\n");
    print_str("Fallos: ");
    print_num(failures);
    print_str("\n");

    if (failures == 0) {
        print_str("STATUS: OK\n");
        lcd_instruccion(0xC0);
        lcd_imprimir("Status: OK");
    } else {
        print_str("STATUS: FAILED\n");
        lcd_instruccion(0xC0);
        lcd_imprimir("Status: FAIL");
    }

    return 0;
}
