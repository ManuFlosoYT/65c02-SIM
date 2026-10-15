#include "BIOS.h"
#include "SD.h"
#include <string.h>

static const char HEX_CHARS[] = "0123456789ABCDEF";

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
    bios_putchar('\r');
    bios_putchar('\n');
}

void read_line(char* buffer, size_t max_len) {
    if (buffer == NULL || max_len == 0) {
        return;
    }
    size_t i = 0;
    while (i + 1 < max_len) {
        char c = 0;
        do {
            c = bios_getchar();
        } while (c == 0);

        if (c == '\r' || c == '\n') {
            bios_putchar('\r');
            bios_putchar('\n');
            break;
        }
        if (c == '\b' || c == 8 || c == 127) {
            if (i > 0) {
                --i;
                bios_putchar(' ');
                bios_putchar('\b');
            }
            continue;
        }
        buffer[i] = c;
        i++;
    }
    buffer[i] = '\0';
}

void print_hex_byte(uint8_t v) {
    uint8_t hi = (uint8_t)(v >> 4U);
    bios_putchar(HEX_CHARS[hi]);
    bios_putchar(HEX_CHARS[v & 0x0FU]);
}

void print_hex(uint16_t n) {
    bios_putchar('0');
    bios_putchar('x');
    if (n > 255U) {
        uint8_t b1 = (uint8_t)((n >> 12U) & 0x0FU);
        uint8_t b2 = (uint8_t)((n >> 8U) & 0x0FU);
        bios_putchar(HEX_CHARS[b1]);
        bios_putchar(HEX_CHARS[b2]);
    }
    uint8_t b3 = (uint8_t)((n >> 4U) & 0x0FU);
    bios_putchar(HEX_CHARS[b3]);
    bios_putchar(HEX_CHARS[n & 0x0FU]);
}

void print_num(uint16_t n) {
    if (n == 0U) {
        bios_putchar('0');
        return;
    }

    char buffer[6];
    uint8_t idx = 0U;
    while (n > 0U) {
        buffer[idx] = (char)((n % 10U) + '0');
        ++idx;
        n /= 10U;
    }
    while (idx > 0U) {
        --idx;
        bios_putchar(buffer[idx]);
    }
}

void delay(uint16_t ms) {
    for (uint16_t i = 0; i < ms; ++i) {
        for (volatile uint16_t j = 0; j < 25U; ++j) {
        }
    }
}

#ifdef ENABLE_SD_SUPPORT

enum SwapConfig {
    SWAP_WINDOW_ADDR = 0x2800U,
    SWAP_WINDOW_SIZE = 0x2000U,
    MAX_SWAP_BLOCKS  = 16U,
};

typedef struct {
    uintptr_t start;
    uint16_t size;
    uint8_t used;
} SwapBlock;

static SwapBlock swap_blocks[MAX_SWAP_BLOCKS];

static const char* const SWP_PFX = "SYSTEM/SWAP/";

void os_clear_swap_blocks(void) {
    swap_blocks[0].start = SWAP_WINDOW_ADDR;
    swap_blocks[0].size = SWAP_WINDOW_SIZE;
    swap_blocks[0].used = 0U;
    for (uint8_t i = 1U; i < MAX_SWAP_BLOCKS; ++i) {
        swap_blocks[i].size = 0U;
        swap_blocks[i].used = 0U;
    }
}

void* os_alloc(uint16_t size) {
    if (size == 0U) {
        return NULL;
    }

    for (uint8_t i = 0U; i < MAX_SWAP_BLOCKS; ++i) {
        if (!swap_blocks[i].used && swap_blocks[i].size >= size) {
            uint16_t remaining = (uint16_t)(swap_blocks[i].size - size);
            swap_blocks[i].used = 1U;
            swap_blocks[i].size = size;

            if (remaining > 0U) {
                for (uint8_t j = 0U; j < MAX_SWAP_BLOCKS; ++j) {
                    if (swap_blocks[j].size == 0U) {
                        swap_blocks[j].start = swap_blocks[i].start + size;
                        swap_blocks[j].size = remaining;
                        swap_blocks[j].used = 0U;
                        break;
                    }
                }
            }
            return (void*)swap_blocks[i].start;
        }
    }
    return NULL;
}

void os_free(void* ptr) {
    for (uint8_t i = 0U; i < MAX_SWAP_BLOCKS; ++i) {
        if (swap_blocks[i].used && swap_blocks[i].start == (uintptr_t)ptr) {
            swap_blocks[i].used = 0U;
            for (uint8_t j = 0U; j < MAX_SWAP_BLOCKS; ++j) {
                if (!swap_blocks[j].used && swap_blocks[j].size > 0U) {
                    if (swap_blocks[i].start + swap_blocks[i].size == swap_blocks[j].start) {
                        swap_blocks[i].size = (uint16_t)(swap_blocks[i].size + swap_blocks[j].size);
                        swap_blocks[j].size = 0U;
                    } else if (swap_blocks[j].start + swap_blocks[j].size == swap_blocks[i].start) {
                        swap_blocks[j].size = (uint16_t)(swap_blocks[j].size + swap_blocks[i].size);
                        swap_blocks[i].size = 0U;
                        i = j;
                    }
                }
            }
            break;
        }
    }
}

static bool is_safe_memory(uintptr_t start, uint16_t size) {
    if (size == 0U) {
        return false;
    }
    uintptr_t end = start + size - 1U;
    if (start >= 0x2800U && end <= 0x47FFU) {
        return true;
    }
    if (start >= 0x6010U && end <= 0x7BFFU) {
        return true;
    }
    return false;
}

static void make_swap_path(char* path, const char* name) {
    strcpy(path, SWP_PFX);
    strncat(path, name, 8U);
    strcat(path, ".SWP");
}

static bool check_addr(void* addr, uint16_t size) {
    if (!is_safe_memory((uintptr_t)addr, size)) {
        print_str("[ERR] Swap addr err\n");
        return false;
    }
    return true;
}

uint8_t os_swap_save(const char* name, void* addr, uint16_t size) {
    char path[32];
    SD_FILE fp;

    if (!check_addr(addr, size)) {
        return 1U;
    }
    make_swap_path(path, name);

    if (sd_open(&fp, path, SD_WRITE | SD_CREATE_ALWAYS)) {
        sd_write(&fp, addr, size);
        sd_close(&fp);
        return 0U;
    }
    return 1U;
}

uint8_t os_swap_load(const char* name, void* addr, uint16_t size) {
    char path[32];
    SD_FILE fp;

    if (!check_addr(addr, size)) {
        return 1U;
    }
    make_swap_path(path, name);

    if (sd_open(&fp, path, SD_READ)) {
        sd_read(&fp, addr, size);
        sd_close(&fp);
        return 0U;
    }
    return 1U;
}

static char msg_buffer[128];

const char* os_get_msg(uint8_t msg_id) {
    SD_FILE f;
    uint16_t offset = 0U;
    uint16_t len = 0U;

    if (!sd_open(&f, "SYSTEM/uDOS.sys", SD_READ)) {
        return "ResErr";
    }

    sd_seek(&f, 1U + ((uint32_t)msg_id * 4U));
    if (sd_read(&f, &offset, 2U) != 2) {
        sd_close(&f);
        return "ResErr";
    }
    if (sd_read(&f, &len, 2U) != 2) {
        sd_close(&f);
        return "ResErr";
    }

    if (len >= sizeof(msg_buffer)) {
        len = (uint16_t)(sizeof(msg_buffer) - 1U);
    }

    sd_seek(&f, offset);
    if (sd_read(&f, msg_buffer, len) != (int16_t)len) {
        sd_close(&f);
        return "ResErr";
    }
    msg_buffer[len] = '\0';

    sd_close(&f);
    return msg_buffer;
}
#endif
