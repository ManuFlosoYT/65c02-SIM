#include "NET.h"

void net_send(const char* s) {
    while (*s) {
        ESP8266_DATA = (uint8_t)*s++;
    }
}

void net_send_num(uint16_t n) {
    char buf[7];
    uint8_t i = 6U;
    buf[i] = '\0';
    if (n == 0U) {
        --i;
        buf[i] = '0';
    } else {
        while (n > 0U && i > 0U) {
            --i;
            buf[i] = (char)((n % 10U) + '0');
            n /= 10U;
        }
    }
    net_send(&buf[i]);
}

void net_cmd(const char* s) {
    net_send(s);
    net_send("\r\n");
}

void net_wifi(const char* ssid, const char* pass) {
    net_send("AT+CWJAP=\"");
    net_send(ssid);
    net_send("\",\"");
    net_send(pass);
    net_cmd("\"");
}

void net_start(const char* type, const char* host, const char* port) {
    net_send("AT+CIPSTART=\"");
    net_send(type);
    net_send("\",\"");
    net_send(host);
    net_send("\",");
    net_cmd(port);
}

void net_cipsend(const char* data, uint16_t len) {
    net_send("AT+CIPSEND=");
    net_send_num(len);
    net_send("\r\n");

    while (true) {
        if (net_has_data() && net_getc() == '>') {
            break;
        }
    }

    while (len > 0U) {
        ESP8266_DATA = (uint8_t)*data++;
        --len;
    }
}
