#include "BIOS.h"
#include "SD.h"
#include <string.h>

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

/* ========================================================================= */
/*                         VIRTUAL PAGED MEMORY & SWAP                       */
/* ========================================================================= */

#define SWAP_WINDOW_ADDR 0x2800
#define SWAP_WINDOW_SIZE 0x2000

#ifdef ENABLE_SD_SUPPORT
/* Asignador de Memoria Dinámica (Swap Heap) */
#define MAX_SWAP_BLOCKS 16

typedef struct {
    uintptr_t start;
    uint16_t size;
    uint8_t used;
} SwapBlock;

static SwapBlock swap_blocks[MAX_SWAP_BLOCKS];

static const char* SYS_DIR = "SYSTEM";
static const char* SWP_DIR = "SYSTEM/SWAP";
static const char* SWP_PFX = "SYSTEM/SWAP/";

void* os_alloc(uint16_t size);
void os_free(void* ptr);

void os_clear_swap_blocks(void) {
    uint8_t i;
    swap_blocks[0].start = SWAP_WINDOW_ADDR;
    swap_blocks[0].size = SWAP_WINDOW_SIZE;
    swap_blocks[0].used = 0;
    for (i = 1; i < MAX_SWAP_BLOCKS; i++) {
        swap_blocks[i].size = 0;
        swap_blocks[i].used = 0;
    }
}

void* os_alloc(uint16_t size) {
    uint8_t i, j;
    if (size == 0) return NULL;
    
    for (i = 0; i < MAX_SWAP_BLOCKS; i++) {
        if (!swap_blocks[i].used && swap_blocks[i].size >= size) {
            uint16_t remaining = swap_blocks[i].size - size;
            swap_blocks[i].used = 1;
            swap_blocks[i].size = size;
            
            if (remaining > 0) {
                for (j = 0; j < MAX_SWAP_BLOCKS; j++) {
                    if (swap_blocks[j].size == 0) {
                        swap_blocks[j].start = swap_blocks[i].start + size;
                        swap_blocks[j].size = remaining;
                        swap_blocks[j].used = 0;
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
    uint8_t i, j;
    for (i = 0; i < MAX_SWAP_BLOCKS; i++) {
        if (swap_blocks[i].used && swap_blocks[i].start == (uintptr_t)ptr) {
            swap_blocks[i].used = 0;
            for (j = 0; j < MAX_SWAP_BLOCKS; j++) {
                if (!swap_blocks[j].used && swap_blocks[j].size > 0) {
                    if (swap_blocks[i].start + swap_blocks[i].size == swap_blocks[j].start) {
                        swap_blocks[i].size += swap_blocks[j].size;
                        swap_blocks[j].size = 0;
                    } else if (swap_blocks[j].start + swap_blocks[j].size == swap_blocks[i].start) {
                        swap_blocks[j].size += swap_blocks[i].size;
                        swap_blocks[i].size = 0;
                        i = j;
                    }
                }
            }
            break;
        }
    }
}

static int is_safe_memory(uintptr_t start, uint16_t size) {
    uintptr_t end = start + size - 1;
    if (size == 0) return 0;
    if (start >= 0x2800 && end <= 0x47FF) return 1;
    if (start >= 0x6010 && end <= 0x7BFF) return 1;
    return 0;
}

static void make_swap_path(char* path, const char* name) {
    strcpy(path, SWP_PFX);
    strncat(path, name, 8);
    strcat(path, ".SWP");
}

static uint8_t check_addr(void* addr, uint16_t size) {
    if (!is_safe_memory((uintptr_t)addr, size)) {
        print_str("[ERR] Swap addr err\n");
        return 0;
    }
    return 1;
}

unsigned char os_swap_save(const char* name, void* addr, unsigned int size) {
    char path[32];
    SD_FILE fp;
    
    if (!check_addr(addr, size)) return 1;
    make_swap_path(path, name);
    
    if (sd_open(&fp, path, SD_WRITE | SD_CREATE_ALWAYS)) {
        sd_write(&fp, addr, size);
        sd_close(&fp);
        return 0;
    }
    return 1;
}

unsigned char os_swap_load(const char* name, void* addr, unsigned int size) {
    char path[32];
    SD_FILE fp;
    
    if (!check_addr(addr, size)) return 1;
    make_swap_path(path, name);
    
    if (sd_open(&fp, path, SD_READ)) {
        sd_read(&fp, addr, size);
        sd_close(&fp);
        return 0;
    }
    return 1;
}

static char msg_buffer[128];

const char* os_get_msg(unsigned char msg_id) {
    SD_FILE f;
    unsigned int offset;
    unsigned int len;
    
    if (!sd_open(&f, "SYSTEM/uDOS.sys", SD_READ)) {
        return "ResErr";
    }
    
    sd_seek(&f, 1 + (msg_id * 4));
    if (sd_read(&f, &offset, 2) != 2) {
        sd_close(&f);
        return "ResErr";
    }
    if (sd_read(&f, &len, 2) != 2) {
        sd_close(&f);
        return "ResErr";
    }
    
    if (len > 511) len = 511;
    
    sd_seek(&f, offset);
    if (sd_read(&f, msg_buffer, len) != len) {
        sd_close(&f);
        return "ResErr";
    }
    msg_buffer[len] = '\0';
    
    sd_close(&f);
    return msg_buffer;
}
#endif
