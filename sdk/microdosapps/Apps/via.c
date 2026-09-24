/* via.c - microDOS command */
#include "../Libs/app-bios.h"
#include <stdint.h>
#include <string.h>

static int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return (*a == *b);
}

static uint16_t parse_hex(const char *str) {
    uint16_t val = 0;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
    }
    while (*str) {
        val <<= 4;
        if (*str >= '0' && *str <= '9') val |= (*str - '0');
        else if (*str >= 'A' && *str <= 'F') val |= (*str - 'A' + 10);
        else if (*str >= 'a' && *str <= 'f') val |= (*str - 'a' + 10);
        str++;
    }
    return val;
}

static volatile uint8_t* get_via_reg(const char *name) {
    if (str_eq(name, "ORB")) return &VIA_ORB;
    if (str_eq(name, "ORA")) return &VIA_ORA;
    if (str_eq(name, "DDRB")) return &VIA_DDRB;
    if (str_eq(name, "DDRA")) return &VIA_DDRA;
    if (str_eq(name, "T1C_L")) return &VIA_T1C_L;
    if (str_eq(name, "T1C_H")) return &VIA_T1C_H;
    if (str_eq(name, "T1L_L")) return &VIA_T1L_L;
    if (str_eq(name, "T1L_H")) return &VIA_T1L_H;
    if (str_eq(name, "T2C_L")) return &VIA_T2C_L;
    if (str_eq(name, "T2C_H")) return &VIA_T2C_H;
    if (str_eq(name, "SR")) return (volatile uint8_t*)0x600A; /* Offset for VIA_SR */
    if (str_eq(name, "ACR")) return &VIA_ACR;
    if (str_eq(name, "PCR")) return &VIA_PCR;
    if (str_eq(name, "IFR")) return &VIA_IFR;
    if (str_eq(name, "IER")) return &VIA_IER;
    return 0;
}

static void print_bin(uint8_t v) {
    int i;
    for (i = 7; i >= 0; i--) {
        bios_putchar((v & (1 << i)) ? '1' : '0');
    }
}

int main(void) {
    char **args = _args_ptr;
    volatile uint8_t *reg;
    uint8_t val;
    
    if (arg_count < 3) {
        println("Usage: via read <REG>");
        println("       via write <REG> <HEX_VAL>");
        return 1;
    }
    
    reg = get_via_reg(args[2]);
    if (!reg) {
        println("Error: Unknown VIA register.");
        return 1;
    }
    
    if (str_eq(args[1], "read")) {
        val = *reg;
        print_str(args[2]);
        print_str(" = 0x");
        print_hex_byte(val);
        print_str(" (0b");
        print_bin(val);
        println(")");
    } else if (str_eq(args[1], "write")) {
        if (arg_count < 4) {
            println("Error: Missing hex value for write.");
            return 1;
        }
        val = (uint8_t)parse_hex(args[3]);
        *reg = val;
        print_str("Written 0x");
        print_hex_byte(val);
        print_str(" to ");
        println(args[2]);
    } else {
        println("Error: Unknown command. Use 'read' or 'write'.");
    }
    
    return 0;
}
