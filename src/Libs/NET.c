#include "NET.h"

void net_send(const char* s) {
    while (*s) ESP8266_DATA = *s++;
}

void net_send_num(uint16_t n) {
    char buf[7];
    int8_t i = 6;
    buf[i--] = '\0';
    if (n == 0) buf[i--] = '0';
    while (n > 0 && i >= 0) {
        buf[i--] = (n % 10) + '0';
        n /= 10;
    }
    net_send(&buf[i + 1]);
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
    
    while (1) {
        if (net_has_data()) {
            if (net_getc() == '>') break;
        }
    }
    
    while (len--) ESP8266_DATA = *data++;
}
