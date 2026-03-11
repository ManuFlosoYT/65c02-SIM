#include "Libs/BIOS.h"
#include "Libs/NET.h"

void wait_and_print(int timeout_ms) {
    int timeout = timeout_ms;
    char c;
    while (timeout > 0) {
        while (net_has_data()) {
            c = net_read_raw_byte();
            bios_putchar(c);
            timeout = timeout_ms; /* Reset timeout as long as we receive data */
        }
        delay(2);
        timeout -= 10;
    }
}

int main(void) {
    INIT_BUFFER();

    print_str("--- ESP8266 Virtual Device Test ---\n");
    print_str("1. Sending AT...\n");
    net_send_cmd("AT");
    wait_and_print(200);

    print_str("\n2. Fetching test.c from GitHub...\n");
    net_send_cmd("AT+HTTPGET=\"https://raw.githubusercontent.com/ManuFlosoYT/65c02-SIM/refs/heads/master/Binaries/test.c\"");
    wait_and_print(1000);

    print_str("\n--- Test Finished ---\n");

    return 0;
}
