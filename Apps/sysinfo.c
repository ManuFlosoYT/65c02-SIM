/* sysinfo.c - microDOS command */
#include "../Libs/app-bios.h"
#include <stdint.h>

int main(void) {
    println("--- System Info ---");

    print_str("App RAM Start : 0x");
    print_hex_byte((APP_SAFE_RAM_START >> 8) & 0xFF);
    print_hex_byte(APP_SAFE_RAM_START & 0xFF);
    println("");

    print_str("App RAM End   : 0x");
    print_hex_byte((APP_SAFE_RAM_END >> 8) & 0xFF);
    print_hex_byte(APP_SAFE_RAM_END & 0xFF);
    println("");

    print_str("App RAM Size  : ");
    print_num(APP_SAFE_RAM_END - APP_SAFE_RAM_START);
    println(" bytes");

    println("SD Free       : N/A (BIOS not impl)");

    return 0;
}
