#include "Include/lcd.h"

unsigned char a;
unsigned char b;
unsigned char next;

int main(void) {
    lcd_init();
    lcd_print("Fibonacci:\n");

    a = 0;
    b = 1;

    /* Infinite loop until 8-bit overflow. */
    while (b >= a) {
        lcd_print_number(a);
        lcd_print(", ");

        next = a + b;

        a = b;
        b = next;
    }

    lcd_print("\nLast: ");
    lcd_print_number(a);
    lcd_print("\n");

    return 0;
}