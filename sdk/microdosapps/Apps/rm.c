/* rm.c - microDOS command */
#include <stdint.h>
#include <string.h>
#include "../Libs/app-bios.h"

#define arg_count  (*(volatile uint8_t*)0x60)
#define _args_ptr  ((char**)(*(uint16_t*)0x61))

int main(void) {
    char** args = _args_ptr;

    if (arg_count < 2) {
        print_str("Usage: ");
        println("rm <path>");
        return 1;
    }
    
    if (sd_remove(args[1])) {
        print_str("Removed: ");
        println(args[1]);
    } else {
        print_str("Error: Could not remove '");
        print_str(args[1]);
        println("'");
        return 1;
    }
    return 0;
}
