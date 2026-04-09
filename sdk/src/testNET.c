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
    uint16_t v = 0;
    const char* p;
    const char* host = "www.google.com";

    INIT_BUFFER();
    print_str("--- ESP TCP 40 BYTES ---\n");

    /* WiFi */
    print_str("WiFi...");
    net_send_cmd_fixed("AT+CWJAP=\"EmulatorNet\",\"password\"\r");
    while (1) {
        if (net_has_data()) {
            char c = net_getc();
            bios_putchar(c);
            if (c == 'K') break; 
        }
    }

    /* TCP */
    print_str("\nTCP...");
    net_send_cmd_fixed("AT+CIPSTART=\"TCP\",\"www.google.com\",80\r");
    while (1) {
        if (net_has_data()) {
            char c = net_getc();
            bios_putchar(c);
            if (c == 'K') break;
        }
    }

    /* GET (Correct 40 bytes) */
    print_str("\nGET...");
    i = 0;
    p = "GET / HTTP/1.0\r\nHost: www.google.com\r\n\r\n";
    while (*p) req[i++] = *p++;
    
    // length is 16 + 22 + 2 = 40 EXACTLY
    net_send_cmd_fixed("AT+CIPSEND=40\r");
    while (!(net_has_data() && net_getc() == '>'));

    for (v = 0; v < i; v++) {
        ESP8266_DATA = req[v];
    }
    
    /* Stream response */
    print_str("\n--- DATA ---\n");
    while (1) {
        if (net_has_data()) {
            bios_putchar(net_getc());
        }
    }

    return 0;
}
