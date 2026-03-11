#ifndef NET_H
#define NET_H

#include <stdint.h>

/* Registers for the ESP8266 */
#define ESP8266_DATA (*(volatile uint8_t*)0x5004)
#define ESP8266_STATUS (*(volatile uint8_t*)0x5005)
#define ESP8266_CMD (*(volatile uint8_t*)0x5006)
#define ESP8266_CTRL (*(volatile uint8_t*)0x5007)

/* Helpers */
#define net_has_data() ((ESP8266_STATUS & 0x80) != 0)
#define net_write_raw_byte(b) (ESP8266_DATA = (b))
#define net_read_raw_byte() (ESP8266_DATA)

static void net_send_string(const char* str) {
    while (*str) {
        ESP8266_DATA = *str++;
    }
}

/* Send an AT command directly. Note that \r\n will be handled by the user or the command */
static void net_send_cmd(const char* cmd) {
    net_send_string(cmd);
    net_send_string("\r\n");
}

#endif /* NET_H */
