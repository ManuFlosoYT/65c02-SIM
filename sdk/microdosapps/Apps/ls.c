/* ls.c - microDOS command */
#include <stdint.h>
#include <string.h>
#include "../Libs/app-bios.h"



int main(void) {
    SD_DIR dir;
    SD_INFO fno;
    char** args = _args_ptr;
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
                print_str(" ");
            }
            println(fno.fname);
        }
        sd_closedir(&dir);
    } else {
        print_str("Error: Could not open directory '");
        print_str(path);
        println("'");
    }
    return 0;
}
