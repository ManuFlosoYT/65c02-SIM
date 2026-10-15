/* nc.c - microDOS command */
#include "../Libs/app-bios.h"
#include <stdint.h>
#include <string.h>

int main(void) {
    char **args = _args_ptr;

    if (arg_count < 3) {
        println("Usage: nc <ip> <port>");
        return 1;
    }

    print_str("Connecting to ");
    print_str(args[1]);
    print_str(":");
    println(args[2]);

    net_send("AT+CIPSTART=\"TCP\",\"");
    net_send(args[1]);
    net_send("\",\"");
    net_send(args[2]);
    net_send("\"\r\n");

    println("Connected. Type to send, Ctrl+C to exit.");

    while (true) {
        if (net_has_data()) {
            char c = (char)net_getc();
            bios_putchar(c);
        }

        char k = bios_getchar();
        if (k != 0) {
            if (k == 3) { /* Ctrl+C */
                println("");
                break;
            }
            net_send("AT+CIPSEND=1\r\n");
            uint16_t d = 0x0FFF;
            while (d--) {
                /* Delay to let ESP process AT+CIPSEND */
            }
            net_putc(k);
            bios_putchar(k);
        }
    }

    net_send("AT+CIPCLOSE\r\n");
    println("Closed.");
    return 0;
}
