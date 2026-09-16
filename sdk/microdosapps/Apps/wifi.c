/* wifi.c - microDOS command */
#include <stdint.h>
#include <string.h>
#include "../Libs/app-bios.h"



int main(void) {
    char chr;
    char** args = _args_ptr;
    
    if (arg_count == 1) {
        println("Scanning for networks...");
        net_cmd("AT+CWLAP");
        while (1) {
            if (net_has_data()) {
                chr = (char)net_getc();
                if (chr == '\r') continue;
                bios_putchar(chr);
                if (chr == 'K') break; /* OK */
                if (chr == 'R') break; /* ERROR */
            }
        }
        return 0;
    }

    if (arg_count < 3) {
        print_str("Usage: ");
        println("wifi <SSID> <Password>");
        return 1;
    }

    print_str("Connecting to ");
    print_str(args[1]);
    println("...");

    net_wifi(args[1], args[2]);

    while (1) {
        if (net_has_data()) {
            chr = (char)net_getc();
            if (chr == '\r') continue;
            bios_putchar(chr);
            if (chr == 'K') { 
                println(""); 
                println("Success: IP obtained"); 
                break; 
            }
            if (chr == 'R') { 
                println(""); 
                println("Error: Could not connect to network"); 
                break; 
            }
        }
    }
    return 0;
}
