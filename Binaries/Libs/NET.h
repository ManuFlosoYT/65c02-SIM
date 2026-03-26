#ifndef NET_H
#define NET_H

#include <stdint.h>

/* Registers for the ESP8266 */
#define ESP8266_DATA   (*(volatile uint8_t*)0x5004)
#define ESP8266_STATUS (*(volatile uint8_t*)0x5005)

/* Fast polling macros */
#define net_has_data() (ESP8266_STATUS & 0x80)
#define net_putc(c)    (ESP8266_DATA = (c))
#define net_getc()     (ESP8266_DATA)

static void net_send(const char* s) {
    while (*s) ESP8266_DATA = *s++;
}

static void net_cmd(const char* s) {
    net_send(s);
    net_send("\r\n");
}

/* Connect to WiFi - standard flow */
static void net_wifi(const char* ssid, const char* pass) {
    net_send("AT+CWJAP=\"");
    net_send(ssid);
    net_send("\",\"");
    net_send(pass);
    net_cmd("\"");
}

/* Start TCP/SSL - standard flow */
static void net_start(const char* type, const char* host, const char* port) {
    net_send("AT+CIPSTART=\"");
    net_send(type);
    net_send("\",\"");
    net_send(host);
    net_send("\",");
    net_cmd(port);
}

/* Send data - handles > prompt */
static void net_cipsend(const char* data, uint16_t len) {
    char buf[8];
    uint16_t v = len;
    uint8_t i = 0;
    
    // Simple int to str
    if (v == 0) buf[i++] = '0';
    else {
        char tmp[8];
        uint8_t j = 0;
        while (v > 0) { tmp[j++] = (v % 10) + '0'; v /= 10; }
        while (j > 0) buf[i++] = tmp[--j];
    }
    buf[i] = '\0';

    net_send("AT+CIPSEND=");
    net_cmd(buf);
    
    // Wait prompt
    while (!(net_has_data() && net_getc() == '>'));
    
    // Send raw
    for (v = 0; v < len; ++v) ESP8266_DATA = data[v];
}

#endif
