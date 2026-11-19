#include "Libs/LCD.h"
#include "Libs/app-bios.h"

int main(void) {
    lcd_init();
    lcd_print("Hello World!");
    println("Hello World!");
    return 0;
}
