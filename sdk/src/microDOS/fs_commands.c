#include "commands.h"
#include "../Libs/BIOS.h"
#include "../Libs/SD.h"
#include "shell.h"
#include "msg.h"

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
                print_str("[D] ");
            } else {
                sz = fno.fsize;
                i = 9;
                size_str[10] = '\0';
                for (; i >= 0; i--) {
                    size_str[i] = (sz > 0 || i == 9) ? (char)((sz % 10) + '0') : ' ';
                    sz /= 10;
                }
                print_str(size_str);
                print_str(M_SPACE);
            }
            println(fno.fname);
        }
        sd_closedir(&dir);
    } else {
        print_str(M_ERR); print_str("Could not open directory '");
        print_str(path); println("'");
    }
}

void cmd_cat(void) {
    SD_FILE fp;
    int c;
    if (arg_count < 2) {
        print_str(M_USE); println("cat <filename>");
        return;
    }
    if (sd_open(&fp, args[1], SD_READ)) {
        while ((c = sd_getc(&fp)) != -1) {
            bios_putchar((char)c);
        }
        println("");
        sd_close(&fp);
    } else {
        print_str(M_ERR); print_str("Could not read file '");
        print_str(args[1]); println("'");
    }
}

void cmd_touch(void) {
    SD_FILE fp;
    if (arg_count < 2) {
        print_str(M_USE); println("touch <filename>");
        return;
    }
    if (sd_open(&fp, args[1], SD_WRITE | SD_CREATE_ALWAYS)) {
        sd_close(&fp);
        print_str("File created: "); println(args[1]);
    } else {
        print_str(M_ERR); print_str("Could not create file '");
        print_str(args[1]); println("'");
    }
}

void cmd_mkdir(void) {
    if (arg_count < 2) {
        print_str(M_USE); println("mkdir <directory>");
        return;
    }
    if (sd_mkdir(args[1])) {
        print_str("Directory created: "); println(args[1]);
    } else {
        print_str(M_ERR); print_str("Could not create directory '");
        print_str(args[1]); println("'");
    }
}

void cmd_cd(void) {
    if (arg_count < 2) {
        print_str(M_USE); println("cd <directory>");
        return;
    }
    if (!sd_chdir(args[1])) {
        print_str(M_ERR); print_str("Could not change directory to '");
        print_str(args[1]); println("'");
    }
}

void cmd_rm(void) {
    if (arg_count < 2) {
        print_str(M_USE); println("rm <path>");
        return;
    }
    if (sd_remove(args[1])) {
        print_str("Removed: "); println(args[1]);
    } else {
        print_str(M_ERR); print_str("Could not remove '");
        print_str(args[1]); println("'");
    }
}
