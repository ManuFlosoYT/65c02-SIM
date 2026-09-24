/* calc.c - microDOS command */
#include "../Libs/app-bios.h"
#include <stdint.h>
#include <string.h>

static uint16_t parse_val(const char *str) {
    uint16_t val = 0;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
        while (*str) {
            val <<= 4;
            if (*str >= '0' && *str <= '9')
                val |= (*str - '0');
            else if (*str >= 'A' && *str <= 'F')
                val |= (*str - 'A' + 10);
            else if (*str >= 'a' && *str <= 'f')
                val |= (*str - 'a' + 10);
            str++;
        }
    } else if (str[0] == '0' && (str[1] == 'b' || str[1] == 'B')) {
        str += 2;
        while (*str) {
            val <<= 1;
            if (*str == '1')
                val |= 1;
            str++;
        }
    } else {
        while (*str) {
            if (*str >= '0' && *str <= '9') {
                val = val * 10 + (*str - '0');
            }
            str++;
        }
    }
    return val;
}

static void print_bin(uint16_t v) {
    int i;
    for (i = 15; i >= 0; i--) {
        bios_putchar((v & (1 << i)) ? '1' : '0');
        if (i == 8)
            bios_putchar('_');
    }
}

int main(void) {
    char **args = _args_ptr;
    uint16_t v1;
    char op;
    uint16_t v2;
    uint16_t res = 0;

    if (arg_count < 4) {
        println("Usage: calc <val1> <op> <val2>");
        println("Ops: + - * / & | ^");
        return 1;
    }

    v1 = parse_val(args[1]);
    op = args[2][0];
    v2 = parse_val(args[3]);

    if (op == '+')
        res = v1 + v2;
    else if (op == '-')
        res = v1 - v2;
    else if (op == '*')
        res = v1 * v2;
    else if (op == '/') {
        if (v2 != 0)
            res = v1 / v2;
        else {
            println("Div by zero");
            return 1;
        }
    } else if (op == '&')
        res = v1 & v2;
    else if (op == '|')
        res = v1 | v2;
    else if (op == '^')
        res = v1 ^ v2;
    else {
        println("Unknown op");
        return 1;
    }

    print_str("Dec: ");
    print_num(res);
    println("");

    print_str("Hex: 0x");
    print_hex_byte((res >> 8) & 0xFF);
    print_hex_byte(res & 0xFF);
    println("");

    print_str("Bin: 0b");
    print_bin(res);
    println("");

    return 0;
}
