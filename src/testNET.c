#include <stdbool.h>
#include <stdint.h>

#include "Libs/BIOS.h"
#include "Libs/NET.h"

void net_send_cmd_fixed(const char* s) {
    while (*s) {
        if (*s != '\n') {
            ESP8266_DATA = *s;
        }
        s++;
    }
}

int main(void) {
    char req[512];
    uint16_t i = 0;
    const char* p = NULL;

    INIT_BUFFER();
    print_str("--- ESP TCP 40 BYTES ---\n");

    print_str("WiFi...");
    net_send_cmd_fixed("AT+CWJAP=\"EmulatorNet\",\"password\"\r");
    while (true) {
        if (net_has_data()) {
            char c = net_getc();
            bios_putchar(c);
            if (c == 'K') {
                break;
            }
        }
    }

    print_str("\nTCP...");
    net_send_cmd_fixed("AT+CIPSTART=\"TCP\",\"www.google.com\",80\r");
    while (true) {
        if (net_has_data()) {
            char c = net_getc();
            bios_putchar(c);
            if (c == 'K') {
                break;
            }
        }
    }

    print_str("\nGET...");
    i = 0;
    p = "GET / HTTP/1.0\r\nHost: www.google.com\r\n\r\n";
    while (*p) {
        req[i++] = *p++;
    }

    net_send_cmd_fixed("AT+CIPSEND=40\r");
    while (!(net_has_data() && net_getc() == '>')) {
    }

    for (uint16_t v = 0; v < i; v++) {
        ESP8266_DATA = req[v];
    }

    print_str("\n--- DATA ---\n");
    while (true) {
        if (net_has_data()) {
            bios_putchar(net_getc());
        }
    }

    return 0;
}
