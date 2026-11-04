#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "../Libs/app-bios.h"

MICRODOS_NO_DESKTOP();

#define SD_CTRL (*(volatile uint8_t*)0x7FC8)

static char path_buf[128];
static uint32_t total_used_bytes = 0;

static void scan_dir(char* path) {
    SD_DIR dir;
    SD_INFO fno;
    uint8_t base_len = (uint8_t)strlen(path);

    if (!sd_opendir(&dir, path)) {
        return;
    }

    while (sd_readdir(&dir, &fno) && fno.fname[0] != '\0') {
        if (fno.fname[0] == '.' && (fno.fname[1] == '\0' || (fno.fname[1] == '.' && fno.fname[2] == '\0'))) {
            continue;
        }

        uint8_t cur = base_len;
        if (cur > 0 && path[cur - 1] != '/' && cur < sizeof(path_buf) - 2) {
            path[cur++] = '/';
        }
        const char* p = fno.fname;
        while (*p && cur < sizeof(path_buf) - 1) {
            path[cur++] = *p++;
        }
        path[cur] = '\0';

        if (fno.fattrib & AM_DIR) {
            scan_dir(path);
        } else {
            uint32_t fsize = fno.fsize;
            uint32_t cluster_bytes = (fsize == 0) ? 0 : (((fsize - 1) / 4096 + 1) * 4096);
            total_used_bytes += cluster_bytes;
        }

        path[base_len] = '\0';
    }

    sd_closedir(&dir);
}

static void format_u32_right(char* buf, uint32_t val, uint8_t width) {
    buf[width] = '\0';
    for (int i = (int)width - 1; i >= 0; i--) {
        buf[i] = (val > 0 || i == (int)width - 1) ? (char)((val % 10) + '0') : ' ';
        val /= 10;
    }
}

static void format_mb_str(char* buf, uint32_t kb) {
    uint32_t tenth = (kb * 10 + 512) / 1024;
    uint32_t whole = tenth / 10;
    uint8_t frac = (uint8_t)(tenth % 10);

    char w_buf[10];
    uint8_t w_len = 0;
    if (whole == 0) {
        w_buf[w_len++] = '0';
    } else {
        while (whole > 0) {
            w_buf[w_len++] = (char)((whole % 10) + '0');
            whole /= 10;
        }
    }
    uint8_t pos = 0;
    while (w_len > 0) {
        buf[pos++] = w_buf[--w_len];
    }
    buf[pos++] = '.';
    buf[pos++] = (char)(frac + '0');
    buf[pos++] = 'M';
    buf[pos] = '\0';
}

static void print_str_right(const char* s, uint8_t width) {
    uint8_t len = (uint8_t)strlen(s);
    for (uint8_t i = len; i < width; i++) {
        bios_putchar(' ');
    }
    print_str(s);
}

int main(void) {
    char** args = _args_ptr;
    bool human_readable = (arg_count > 1 && strcmp(args[1], "-h") == 0);

    if ((SD_CTRL & 0x01) == 0) {
        println("df: SD card not mounted");
        return 1;
    }

    total_used_bytes = 0;
    path_buf[0] = '/';
    path_buf[1] = '\0';
    scan_dir(path_buf);

    uint32_t total_kb = 32768;
    uint32_t used_kb = total_used_bytes / 1024;
    if (used_kb > total_kb) used_kb = total_kb;
    uint32_t avail_kb = total_kb - used_kb;
    uint32_t pct = (uint32_t)((used_kb * 100) / total_kb);

    char num_buf[12];

    if (human_readable) {
        println("Filesystem          Size      Used     Avail Use% Mounted on");
        print_str("/dev/sd0           ");
        print_str_right("32.0M", 10);

        char used_mb[12];
        char avail_mb[12];
        format_mb_str(used_mb, used_kb);
        format_mb_str(avail_mb, avail_kb);

        print_str_right(used_mb, 10);
        print_str_right(avail_mb, 10);

        format_u32_right(num_buf, pct, 4);
        print_str(num_buf);
        bios_putchar('%');

        println(" /");
    } else {
        println("Filesystem     1K-blocks      Used Available Use% Mounted on");
        print_str("/dev/sd0           32768");

        format_u32_right(num_buf, used_kb, 10);
        print_str(num_buf);

        format_u32_right(num_buf, avail_kb, 10);
        print_str(num_buf);

        format_u32_right(num_buf, pct, 4);
        print_str(num_buf);
        bios_putchar('%');

        println(" /");
    }

    return 0;
}
