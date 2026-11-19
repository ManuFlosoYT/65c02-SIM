#include "../Libs/app-bios.h"

static uint16_t xam;
static uint16_t st;
static bool has_xam;

static void print_addr(uint16_t addr) {
    print_hex_byte((uint8_t)(addr >> 8));
    print_hex_byte((uint8_t)(addr & 0xFF));
}

static void dump_range(uint16_t start, uint16_t end) {
    if (start > end) {
        return;
    }
    uint16_t addr = start;
    while (true) {
        if (addr == start || (addr & 7) == 0) {
            if (addr != start) {
                bios_putchar('\r');
                bios_putchar('\n');
            }
            print_addr(addr);
            bios_putchar(':');
        }
        bios_putchar(' ');
        print_hex_byte(*(volatile uint8_t *)addr);
        if (addr == end) {
            break;
        }
        addr++;
    }
    bios_putchar('\r');
    bios_putchar('\n');
    xam = end;
    has_xam = true;
}

static bool is_hex_digit(char c) {
    return (c >= '0' && c <= '9') ||
           (c >= 'A' && c <= 'F') ||
           (c >= 'a' && c <= 'f');
}

static uint8_t hex_val(char c) {
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    return 0;
}

static uint16_t parse_hex(const char **p) {
    uint16_t val = 0;
    const char *s = *p;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }
    while (is_hex_digit(*s)) {
        val = (uint16_t)((val << 4) | hex_val(*s));
        s++;
    }
    *p = s;
    return val;
}

static void process_line(const char *line) {
    const char *p = line;
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p == '\0') {
        uint16_t start = has_xam ? (uint16_t)(xam + 1) : 0x0000;
        uint16_t end = (uint16_t)(start + 7);
        if (end < start) {
            end = 0xFFFF;
        }
        dump_range(start, end);
        return;
    }

    while (*p) {
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (!*p) {
            break;
        }

        if (*p == '.') {
            p++;
            while (*p == ' ' || *p == '\t') p++;
            if (is_hex_digit(*p)) {
                uint16_t end = parse_hex(&p);
                dump_range(xam, end);
            }
            continue;
        }

        if (*p == ':') {
            p++;
            while (true) {
                while (*p == ' ' || *p == '\t') p++;
                if (!is_hex_digit(*p)) break;
                uint8_t b = (uint8_t)parse_hex(&p);
                *(volatile uint8_t *)st = b;
                xam = st;
                has_xam = true;
                st++;
            }
            continue;
        }

        if (*p == 'R' || *p == 'r') {
            p++;
            ((void (*)(void))xam)();
            continue;
        }

        if (is_hex_digit(*p)) {
            uint16_t val = parse_hex(&p);
            while (*p == ' ' || *p == '\t') p++;

            if (*p == '.') {
                p++;
                while (*p == ' ' || *p == '\t') p++;
                if (is_hex_digit(*p)) {
                    uint16_t end = parse_hex(&p);
                    dump_range(val, end);
                }
            } else if (*p == ':') {
                p++;
                st = val;
                while (true) {
                    while (*p == ' ' || *p == '\t') p++;
                    if (!is_hex_digit(*p)) break;
                    uint8_t b = (uint8_t)parse_hex(&p);
                    *(volatile uint8_t *)st = b;
                    xam = st;
                    has_xam = true;
                    st++;
                }
            } else if (*p == 'R' || *p == 'r') {
                p++;
                xam = val;
                has_xam = true;
                ((void (*)(void))val)();
            } else {
                dump_range(val, val);
            }
            continue;
        }

        bios_putchar('\\');
        bios_putchar('\r');
        bios_putchar('\n');
        break;
    }
}

static int read_line(char *buf, uint8_t max_len) {
    uint8_t len = 0;
    while (true) {
        char c = bios_getchar();
        if (c == 0x03) {
            bios_putchar('\r');
            bios_putchar('\n');
            return -1;
        }
        if (c == 27 || c == '\\') {
            bios_putchar('\\');
            bios_putchar('\r');
            bios_putchar('\n');
            len = 0;
            continue;
        }
        if (c == '\b' || c == 8 || c == 127) {
            if (len > 0) {
                len--;
                bios_putchar('\b');
                bios_putchar(' ');
                bios_putchar('\b');
            }
            continue;
        }
        if (c == '\r' || c == '\n') {
            bios_putchar('\r');
            bios_putchar('\n');
            buf[len] = '\0';
            return (int)len;
        }
        if (len < max_len - 1) {
            if (c >= 'a' && c <= 'z') {
                c -= ('a' - 'A');
            }
            buf[len++] = c;
        }
    }
}

int main(void) {
    char line[128];

    println("WOZMON - Press Ctrl+C to exit\r\n");
    bios_putchar('\\');
    bios_putchar('\r');
    bios_putchar('\n');

    while (true) {
        int len = read_line(line, sizeof(line));
        if (len < 0) {
            return 0;
        }
        process_line(line);
    }
}
