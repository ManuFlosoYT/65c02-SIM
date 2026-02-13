#include "Include/bios.h"
#include "Include/lcd.h"

int main(void) {
    unsigned int n;

    // ---------------------------------------------------------
    // 1. INITIALIZATION
    // ---------------------------------------------------------
    print_str("[BIOS] Start.\n");

    lcd_init();
    lcd_print("Started...");

    print_str("[BIOS] LCD ready. Waiting...\n");
    delay(1000);

    // ---------------------------------------------------------
    // 2. TEXT AND NEWLINE
    // ---------------------------------------------------------
    lcd_clear(); /* Clear previous */

    lcd_print("Line 1");
    lcd_newline();
    lcd_print("Line 2");

    print_str("[BIOS] Text written (2 lines).\n");
    delay(1000);

    // ---------------------------------------------------------
    // 3. CLEAR TEST
    // ---------------------------------------------------------
    lcd_clear();

    print_str("[BIOS] Screen Clear.\n");
    /* Pause to see empty screen */
    delay(1000);

    // ---------------------------------------------------------
    // 4. NUMBER COUNTING
    // ---------------------------------------------------------
    lcd_print("Nums: ");

    for (n = 0; n <= 9; n++) {
        lcd_print_number(n);
        lcd_print_char(' ');

        bios_putchar('.'); /* Visual feedback in console */

        /* Use massive delay between EACH number */
        delay(1000);
    }

    bios_putchar('\n');

    // ---------------------------------------------------------
    // 5. END
    // ---------------------------------------------------------
    lcd_clear();
    lcd_print("End.");
    print_str("[BIOS] Program end.\n");

    return 0;
}