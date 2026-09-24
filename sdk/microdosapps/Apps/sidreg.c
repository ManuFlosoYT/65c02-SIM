/* sidreg.c - microDOS command */
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
    uint8_t reg;
    uint8_t val;

    if (arg_count < 3) {
        println("Usage: sidreg <reg_hex> <val_hex>");
        println("Example: sidreg 0x18 0x0F");
        return 1;
    }

    reg = (uint8_t)parse_hex(args[1]);
    val = (uint8_t)parse_hex(args[2]);

    if (reg > 28) {
        println("Error: SID reg 0x00 to 0x1C");
        return 1;
    }

    sid_write(reg, val);

    print_str("SID[0x");
    print_hex_byte(reg);
    print_str("] = 0x");
    print_hex_byte(val);
    println("");

    return 0;
}
