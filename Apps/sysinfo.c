/* sysinfo.c - microDOS command */
#include "../Libs/app-bios.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ACIA_STATUS (*(volatile uint8_t*)0x7FC1)
#define ACIA_CMD    (*(volatile uint8_t*)0x7FC2)
#define ACIA_CTRL   (*(volatile uint8_t*)0x7FC3)
#define SD_CTRL     (*(volatile uint8_t*)0x7FC8)

static SD_DIR dir;
static SD_INFO fno;
static char cwd[32];

static const uint16_t baud_rates[16] = {
    0, 50, 75, 110, 134, 150, 300, 600, 1200, 1800, 2400, 3600, 4800, 7200, 9600, 19200
};

static void print_hex_word(uint16_t w) {
    print_hex_byte((uint8_t)(w >> 8));
    print_hex_byte((uint8_t)(w & 0xFF));
}

static uint8_t get_sp(void) {
    uint8_t sp;
    __asm__ volatile (
        "tsx\n\t"
        "stx %0"
        : "=m"(sp)
        :
        : "x"
    );
    return sp;
}

static uint8_t get_flags(void) {
    uint8_t p;
    __asm__ volatile (
        "php\n\t"
        "pla\n\t"
        "sta %0"
        : "=m"(p)
        :
        : "a"
    );
    return p;
}

static void print_flags(uint8_t f) {
    bios_putchar('[');
    bios_putchar((f & 0x80) ? 'N' : '-');
    bios_putchar((f & 0x40) ? 'V' : '-');
    bios_putchar('-');
    bios_putchar((f & 0x10) ? 'B' : '-');
    bios_putchar((f & 0x08) ? 'D' : '-');
    bios_putchar((f & 0x04) ? 'I' : '-');
    bios_putchar((f & 0x02) ? 'Z' : '-');
    bios_putchar((f & 0x01) ? 'C' : '-');
    bios_putchar(']');
}

int main(void) {
    println("=== System Info ===");

    println("--- CPU & Architecture ---");
    println("CPU Model     : WDC W65C02S");

    print_str("Stack Pointer : 0x01");
    print_hex_byte(get_sp());
    println("");

    uint8_t p = get_flags();
    print_str("Flags (P)     : 0x");
    print_hex_byte(p);
    print_str(" ");
    print_flags(p);
    println("");

    println("Zero Page     : 0x0000 - 0x00FF (256 B)");
    println("  MOS Virtual : 0x0000 - 0x001F (32 B)");
    println("  uDOS State  : 0x0060 - 0x00FD");

    println("--- Memory Layout ---");
    println("Total RAM     : 32 KB (0x0000-0x7FFF)");

    print_str("App Safe RAM  : 0x");
    print_hex_word(APP_SAFE_RAM_START);
    print_str(" - 0x");
    print_hex_word(APP_SAFE_RAM_END);
    print_str(" (");
    print_num(APP_SAFE_RAM_END - APP_SAFE_RAM_START + 1);
    println(" B)");

    print_str("Swap Window   : 0x");
    print_hex_word((uint16_t)(uintptr_t)SWAP_WINDOW_ADDR);
    print_str(" (8 KB)");
    println("");

    println("Kernel RAM    : 0x5400 - 0x5FBF (3 KB)");

    print_str("Extra/VRAM    : 0x");
    print_hex_word(APP_EXTRA_RAM_START);
    print_str(" - 0x");
    print_hex_word(APP_EXTRA_RAM_END);
    println(" (8 KB)");

    println("MMIO Space    : 0x7FC0 - 0x7FFF (64 B)");
    println("System ROM    : 0x8000 - 0xFFFF (32 KB)");

    print_str("Vectors (N/R/I): 0x");
    print_hex_word(*(volatile uint16_t*)0xFFFA);
    print_str(" 0x");
    print_hex_word(*(volatile uint16_t*)0xFFFC);
    print_str(" 0x");
    print_hex_word(*(volatile uint16_t*)0xFFFE);
    println("");

    println("--- Peripherals & MMIO ---");
    uint8_t acia_ctrl = ACIA_CTRL;
    uint8_t baud_idx = acia_ctrl & 0x0F;
    print_str("ACIA (0x7FC0) : Status 0x");
    print_hex_byte(ACIA_STATUS);
    print_str(" (");
    print_num(baud_rates[baud_idx]);
    println(" baud)");

    print_str("VIA  (0x7FD0) : DDRB 0x");
    print_hex_byte(VIA_DDRB);
    print_str(" DDRA 0x");
    print_hex_byte(VIA_DDRA);
    print_str(" T1 0x");
    uint16_t t1 = (uint16_t)VIA_T1C_L | ((uint16_t)VIA_T1C_H << 8);
    print_hex_word(t1);
    println("");

    println("SID  (0x7FE0) : 3 Voices, 29 Regs");

    print_str("GPU  (0x5FC0) : Text 100x75 / 320x200");
    println("");

    print_str("Mouse (PS/2)  : X=");
    print_num(mouse_get_x());
    print_str(" Y=");
    print_num(mouse_get_y());
    print_str(" Btn=0x");
    print_hex_byte(MOUSE_BTNS);
    println("");

    print_str("WiFi (0x7FC4) : ESP8266 (Status 0x");
    print_hex_byte(ESP8266_STATUS);
    println(")");

    println("--- Storage & Filesystem ---");
    bool sd_mounted = (SD_CTRL & 0x01) != 0;
    print_str("SD Card       : ");
    if (sd_mounted) {
        println("Mounted (FAT16)");
    } else {
        println("Not Mounted");
    }

    if (sd_getcwd(cwd, sizeof(cwd))) {
        print_str("Working Dir   : ");
        println(cwd);
    }

    if (sd_opendir(&dir, "/bin")) {
        uint16_t app_count = 0;
        uint32_t total_bytes = 0;
        while (sd_readdir(&dir, &fno)) {
            if (!(fno.fattrib & AM_DIR)) {
                app_count++;
                total_bytes += fno.fsize;
            }
        }
        sd_closedir(&dir);
        print_str("Apps in /bin  : ");
        print_num(app_count);
        print_str(" (");
        print_num((uint16_t)(total_bytes / 1024));
        println(" KB)");
    }

    void* test_mem = os_alloc(512);
    print_str("Heap Allocator: ");
    if (test_mem) {
        println("OK (512 B test)");
        os_free(test_mem);
    } else {
        println("Unavailable");
    }

    return 0;
}
