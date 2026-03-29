#include "BIOS.h"

const char HEX_CHARS[] = "0123456789ABCDEF";

void print_str(const char* s) {
    while (*s) {
        if (*s == '\n') {
            bios_putchar('\r');
            bios_putchar('\n');
        } else {
            bios_putchar(*s);
        }
        s++;
    }
}

void println(const char* s) {
    print_str(s);
    bios_putchar('\n');
}

void read_line(char* buffer, int max_len) {
    char c;
    int i = 0;
    while (i < max_len - 1) {
        do {
            c = bios_getchar();
        } while (c == 0);
        if (c == '\r' || c == '\n') {
            bios_putchar('\r');
            bios_putchar('\n');
            break;
        }
        buffer[i] = c;
        i++;
    }
    buffer[i] = '\0';
}

void print_hex_byte(unsigned char v) {
    bios_putchar(HEX_CHARS[(v >> 4) & 0xF]);
    bios_putchar(HEX_CHARS[v & 0xF]);
}

void print_hex(unsigned int n) {
    bios_putchar('0');
    bios_putchar('x');
    if (n > 255) {
        bios_putchar(HEX_CHARS[(n >> 12) & 0xF]);
        bios_putchar(HEX_CHARS[(n >> 8) & 0xF]);
    }
    bios_putchar(HEX_CHARS[(n >> 4) & 0xF]);
    bios_putchar(HEX_CHARS[n & 0xF]);
}

void print_num(unsigned int n) {
    char buffer[10];
    int idx = 0;
    if (n == 0) {
        bios_putchar('0');
        return;
    }
    while (n > 0) {
        buffer[idx++] = (n % 10) + '0';
        n /= 10;
    }
    while (idx > 0) {
        bios_putchar(buffer[--idx]);
    }
}

void delay(unsigned int ms) {
    volatile unsigned int i;
    volatile unsigned int j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 4000; j++) {
        }
    }
}
