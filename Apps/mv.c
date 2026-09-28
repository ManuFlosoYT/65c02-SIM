/* mv.c - microDOS command */
#include "../Libs/app-bios.h"
#include <stdint.h>
#include <string.h>

int main(void) {
    SD_FILE fp_src;
    SD_FILE fp_dst;
    char **args = _args_ptr;
    uint8_t buf[512];
    int16_t bytes_read;
    int16_t bytes_written;
    uint8_t error = 0;

    if (arg_count < 3) {
        println("Usage: mv <src> <dst>");
        return 1;
    }

    if (!sd_open(&fp_src, args[1], SD_READ)) {
        println("Error: Source not found.");
        return 1;
    }

    if (!sd_open(&fp_dst, args[2], SD_WRITE | SD_CREATE_ALWAYS)) {
        sd_close(&fp_src);
        println("Error: Cannot create dst.");
        return 1;
    }

    while ((bytes_read = sd_read(&fp_src, buf, sizeof(buf))) > 0) {
        bytes_written = sd_write(&fp_dst, buf, bytes_read);
        if (bytes_written != bytes_read) {
            println("Error: Write failed.");
            error = 1;
            break;
        }
    }

    sd_close(&fp_dst);
    sd_close(&fp_src);

    if (!error) {
        sd_remove(args[1]);
    } else {
        sd_remove(args[2]);
    }

    return error ? 1 : 0;
}
