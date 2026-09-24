/* peek.c - microDOS command */
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
    uint8_t *ptr;
    uint8_t val;

    if (arg_count < 2) {
        println("Usage: peek <hex_addr>");
        return 1;
    }

    addr = parse_hex(args[1]);
    ptr = (uint8_t *)addr;
    val = *ptr;

    print_str("0x");
    print_hex_byte((addr >> 8) & 0xFF);
    print_hex_byte(addr & 0xFF);
    print_str(": 0x");
    print_hex_byte(val);
    println("");

    return 0;
}
