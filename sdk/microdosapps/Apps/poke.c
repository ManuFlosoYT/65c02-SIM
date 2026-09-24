/* poke.c - microDOS command */
#include "../Libs/app-bios.h"
#include <stdint.h>

static uint16_t parse_hex(const char *str) {
    uint16_t val = 0;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
    }
    while (*str) {
        val <<= 4;
        if (*str >= '0' && *str <= '9') {
            val |= (*str - '0');
        } else if (*str >= 'A' && *str <= 'F') {
            val |= (*str - 'A' + 10);
        } else if (*str >= 'a' && *str <= 'f') {
            val |= (*str - 'a' + 10);
        }
        str++;
    }
    return val;
}

int main(void) {
    char **args = _args_ptr;
    uint16_t addr;
    uint8_t val;
    uint8_t *ptr;

    if (arg_count < 3) {
        println("Usage: poke <hex_addr> <hex_val>");
        return 1;
    }

    addr = parse_hex(args[1]);
    val = (uint8_t)parse_hex(args[2]);
    ptr = (uint8_t *)addr;

    *ptr = val;

    print_str("Written 0x");
    print_hex_byte(val);
    print_str(" to 0x");
    print_hex_byte((addr >> 8) & 0xFF);
    print_hex_byte(addr & 0xFF);
    println("");

    return 0;
}
