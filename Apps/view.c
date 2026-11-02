#include <stdint.h>
#include <string.h>
#include "Libs/app-bios.h"
#include "Libs/GPU.h"

MICRODOS_NO_DESKTOP();

static char to_lower(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}

static int strcasecmp_custom(const char* s1, const char* s2) {
    while (*s1 && (to_lower(*s1) == to_lower(*s2))) {
        s1++;
        s2++;
    }
    return (uint8_t)to_lower(*s1) - (uint8_t)to_lower(*s2);
}

static void list_vram(void) {
    SD_DIR dp;
    SD_INFO fno;
    println("Images in /vram:");
    if (sd_opendir(&dp, "/vram")) {
        while (sd_readdir(&dp, &fno)) {
            if (!(fno.fattrib & AM_DIR)) {
                println(fno.fname);
            }
        }
        sd_closedir(&dp);
    }
}

int main(void) {
    char** args = _args_ptr;
    SD_FILE file;
    char path[80];

    if (arg_count < 2) {
        println("Usage: view <image>");
        list_vram();
        return 1;
    }

    if (strcmp(args[1], "clear") == 0) {
        gpu_fill_screen(0x00);
        return 0;
    }

    if (sd_open(&file, args[1], SD_READ)) {
        goto draw;
    }

    strcpy(path, "/vram/");
    strcat(path, args[1]);
    if (sd_open(&file, path, SD_READ)) {
        goto draw;
    }

    strcat(path, ".bin");
    if (sd_open(&file, path, SD_READ)) {
        goto draw;
    }

    {
        SD_DIR dp;
        SD_INFO fno;
        uint8_t found = 0;
        char name_bin[64];
        const char* name = args[1];

        for (const char* p = args[1]; *p; p++) {
            if (*p == '/') {
                name = p + 1;
            }
        }

        strcpy(name_bin, name);
        strcat(name_bin, ".bin");

        if (sd_opendir(&dp, "/vram")) {
            while (sd_readdir(&dp, &fno)) {
                if (fno.fattrib & AM_DIR) {
                    continue;
                }
                if (strcasecmp_custom(fno.fname, name) == 0 ||
                    strcasecmp_custom(fno.fname, name_bin) == 0) {
                    strcpy(path, "/vram/");
                    strcat(path, fno.fname);
                    found = 1;
                    break;
                }
            }
            sd_closedir(&dp);
        }

        if (found && sd_open(&file, path, SD_READ)) {
            goto draw;
        }
    }

    print_str("Error: Image not found: ");
    println(args[1]);
    list_vram();
    return 1;

draw: {
        uint16_t loaded = 0;
        int16_t n = 0;
        while (loaded < 8192 && (n = sd_read(&file, (uint8_t*)GPU_VRAM_START + loaded, 512)) > 0) {
            loaded += (uint16_t)n;
        }
        sd_close(&file);
        println("Done.");
        return 0;
    }
}
