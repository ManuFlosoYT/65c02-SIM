#include <string.h>
#include "Libs/BIOS.h"
#include "Libs/SD.h"
#include "shell.h"
#include "commands.h"

void cmd_ls(void) {
    SD_DIR dir;
    SD_INFO fno;
    char* path;
    char size_str[11];
    unsigned long sz;
    int i;

    path = (arg_count > 1) ? args[1] : ".";

    if (sd_opendir(&dir, path)) {
        while (sd_readdir(&dir, &fno)) {
            if (fno.fattrib & AM_DIR) {
                print_str("[DIR]      ");
            } else {
                sz = fno.fsize;
                i = 9;
                size_str[10] = '\0';
                for (; i >= 0; i--) {
                    size_str[i] = (sz > 0 || i == 9) ? (char)((sz % 10) + '0') : ' ';
                    sz /= 10;
                }
                print_str(size_str);
                print_str(" ");
            }
            println(fno.fname);
        }
        sd_closedir(&dir);
    } else {
        println("ERROR: ls failed");
    }
}

void cmd_cat(void) {
    SD_FILE fp;
    int c;
    if (arg_count < 2) {
        println("Usage: cat <file>");
        return;
    }
    if (sd_open(&fp, args[1], SD_READ)) {
        while ((c = sd_getc(&fp)) != -1) {
            bios_putchar((char)c);
        }
        println("");
        sd_close(&fp);
    } else {
        println("ERROR: cat failed");
    }
}

void cmd_touch(void) {
    SD_FILE fp;
    if (arg_count < 2) {
        println("Usage: touch <file>");
        return;
    }
    if (sd_open(&fp, args[1], SD_WRITE | SD_CREATE_ALWAYS)) {
        sd_close(&fp);
    } else {
        println("ERROR: touch failed");
    }
}

void cmd_mkdir(void) {
    if (arg_count < 2) {
        println("Usage: mkdir <dir>");
        return;
    }
    if (!sd_mkdir(args[1])) {
        println("ERROR: mkdir failed");
    }
}

void cmd_cd(void) {
    if (arg_count < 2) {
        println("Usage: cd <dir>");
        return;
    }
    if (!sd_chdir(args[1])) {
        println("ERROR: cd failed");
    }
}

void cmd_rm(void) {
    if (arg_count < 2) {
        println("Usage: rm <path>");
        return;
    }
    if (!sd_remove(args[1])) {
        println("ERROR: rm failed");
    }
}

void cmd_nano(void) {
    SD_FILE fp;
    char c;
    static char editor_buf[4096];
    int editor_len = 0;

    if (arg_count < 2) {
        println("Usage: nano <file>");
        return;
    }

    if (!sd_open(&fp, args[1], SD_WRITE | SD_CREATE_ALWAYS)) {
        println("ERROR: nano failed (open)");
        return;
    }

    println("--- NANO EDITOR ---");
    println("Type your text. Press ESC to save and exit.");

    while (1) {
        c = bios_getchar();
        if (c == 27) break;

        if (c == '\b' || c == 8 || c == 127) {
            if (editor_len > 0) {
                editor_len--;
                bios_putchar(' ');
                bios_putchar('\b');
            }
        } else if (c == '\r' || c == '\n') {
            if (editor_len < sizeof(editor_buf) - 1) {
                editor_buf[editor_len++] = '\n';
                bios_putchar('\r');
                bios_putchar('\n');
            }
        } else {
            if (editor_len < sizeof(editor_buf) - 1) {
                editor_buf[editor_len++] = c;
            }
        }
    }

    editor_buf[editor_len] = '\0';
    sd_puts(&fp, editor_buf);
    
    sd_close(&fp);
    println("");
    println("--- SAVED ---");
}