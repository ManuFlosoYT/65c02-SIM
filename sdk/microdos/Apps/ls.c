/* ls.c - microDOS command */
#include <stdint.h>
#include <string.h>
#include "../Libs/app-bios.h"

MICRODOS_NO_DESKTOP();

static void format_u32_right(char* buf, uint32_t val, uint8_t width) {
    buf[width] = '\0';
    for (int i = (int)width - 1; i >= 0; i--) {
        buf[i] = (val > 0 || i == (int)width - 1) ? (char)((val % 10) + '0') : ' ';
        val /= 10;
    }
}

int main(void) {
    SD_DIR dir;
    SD_INFO fno;
    char** args = _args_ptr;
    char* path = (arg_count > 1) ? args[1] : ".";
    char b_buf[11];
    char kb_buf[8];

    if (sd_opendir(&dir, path)) {
        while (sd_readdir(&dir, &fno) && fno.fname[0] != '\0') {
            if (fno.fattrib & AM_DIR) {
                print_str("[DIR ]         - B        - KB  ");
            } else {
                format_u32_right(b_buf, fno.fsize, 9);
                format_u32_right(kb_buf, (fno.fsize == 0) ? 0 : ((fno.fsize - 1) / 1024 + 1), 6);
                print_str("[FILE] ");
                print_str(b_buf);
                print_str(" B ");
                print_str(kb_buf);
                print_str(" KB  ");
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
