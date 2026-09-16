/* touch.c - microDOS command */
#include <stdint.h>
#include <string.h>
#include "../Libs/app-bios.h"



int main(void) {
    SD_FILE fp;
    char** args = _args_ptr;

    if (arg_count < 2) {
        print_str("Usage: ");
        println("touch <filename>");
        return 1;
    }
    
    if (sd_open(&fp, args[1], SD_WRITE | SD_CREATE_ALWAYS)) {
        sd_close(&fp);
        print_str("File created: ");
        println(args[1]);
    } else {
        print_str("Error: Could not create file '");
        print_str(args[1]);
        println("'");
        return 1;
    }
    return 0;
}
