/*
 * ping.c — microDOS ICMP ping app
 * Compile with: ./compile-bin.sh ping --microDOS
 */
#include <stdint.h>
#include <string.h>
#include "Libs/app-bios.h"

#define arg_count  (*(volatile uint8_t*)0x60)
#define _args_ptr  ((char**)(*(uint16_t*)0x61))

int main(void) {
    uint8_t i;
    uint16_t min_ping = 65535U, max_ping = 0, total_ping = 0;
    uint8_t lost = 0;
    uint16_t current;
    char chr;
    char** args = _args_ptr;

    if (arg_count < 2) {
        print_str("Usage: "); println("ping <host>");
        return 1;
    }

    print_str("Pinging ");
    print_str(args[1]);
    println(":");

    for (i = 0; i < 10; ++i) {
        net_send("AT+PING=\"");
        net_send(args[1]);
        net_cmd("\"");

        while (1) {
            if (net_has_data()) {
                chr = (char)net_getc();
                if (chr == '+') {
                    while (!net_has_data());
                    chr = (char)net_getc();
                    if (chr >= '0' && chr <= '9') {
                        current = (uint16_t)(chr - '0');
                        while (1) {
                            if (net_has_data()) {
                                chr = (char)net_getc();
                                if (chr >= '0' && chr <= '9')
                                    current = current * 10 + (uint16_t)(chr - '0');
                                else break;
                            }
                        }
                        print_str("From ");
                        print_str(args[1]);
                        print_str(": ");
                        print_num(current);
                        println("ms");
                        if (current < min_ping) min_ping = current;
                        if (current > max_ping) max_ping = current;
                        total_ping += current;
                        goto next_ping;
                    } else if (chr == 't') {
                        print_str("From ");
                        print_str(args[1]);
                        println(": Timeout");
                        lost++;
                        goto next_ping;
                    }
                } else if (chr == 'E') {
                    print_str("From ");
                    print_str(args[1]);
                    println(": Network Error");
                    lost++;
                    goto next_ping;
                }
            }
        }
    next_ping:
        while (1) {
            if (net_has_data()) {
                chr = (char)net_getc();
                if (chr == 'K' || chr == 'R') break;
            }
        }
    }

    println("\nPing:");
    print_str("  Sent 10, Recv ");
    print_num(10 - lost);
    print_str(", Lost ");
    print_num(lost);
    print_str(" (");
    print_num(lost * 10);
    println("% loss)");

    if (lost < 10) {
        println("Times (ms):");
        print_str("  Min=");
        print_num(min_ping);
        print_str("ms, Max=");
        print_num(max_ping);
        print_str("ms, Avg=");
        print_num(total_ping / (10 - lost));
        println("ms");
    }
    return 0;
}
