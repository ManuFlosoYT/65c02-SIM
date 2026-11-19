#include <stdbool.h>
#include <stdint.h>
#include "../Libs/app-bios.h"

MICRODOS_NO_DESKTOP();

int main(void) {
    char** args = _args_ptr;

    if (arg_count < 2) {
        println("Usage: rmdir <directory> [dir2...]");
        return 1;
    }

    uint8_t has_error = 0;
    for (uint8_t i = 1; i < arg_count; i++) {
        const char* arg = args[i];
        SD_DIR dir;
        SD_INFO fno;

        if (!sd_opendir(&dir, arg)) {
            print_str("rmdir: '");
            print_str(arg);
            println("': No such directory");
            has_error = 1;
            continue;
        }

        bool empty = true;
        while (sd_readdir(&dir, &fno) && fno.fname[0] != '\0') {
            if (fno.fname[0] == '.' && (fno.fname[1] == '\0' || (fno.fname[1] == '.' && fno.fname[2] == '\0'))) {
                continue;
            }
            empty = false;
            break;
        }

        sd_closedir(&dir);

        if (!empty) {
            print_str("rmdir: '");
            print_str(arg);
            println("': Directory not empty");
            has_error = 1;
            continue;
        }

        if (!sd_remove(arg)) {
            print_str("rmdir: failed to remove '");
            print_str(arg);
            println("'");
            has_error = 1;
        }
    }

    return has_error ? 1 : 0;
}
