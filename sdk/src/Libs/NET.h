#pragma once

#include <stdbool.h>
#include <stdint.h>

#if defined(MICRODOS) || defined(__MICRODOS__)
#define ESP8266_DATA   (*(volatile uint8_t*)0x7FC4)
#define ESP8266_STATUS (*(volatile uint8_t*)0x7FC5)
#else
#define ESP8266_DATA   (*(volatile uint8_t*)0x5004)
#define ESP8266_STATUS (*(volatile uint8_t*)0x5005)
#endif

static inline bool net_has_data(void) {
    return (ESP8266_STATUS & 0x80U) != 0U;
}

static inline void net_putc(uint8_t c) {
    ESP8266_DATA = c;
}

static inline uint8_t net_getc(void) {
    return ESP8266_DATA;
}

void net_send(const char* s);
void net_send_num(uint16_t n);
void net_cmd(const char* s);
void net_wifi(const char* ssid, const char* pass);
void net_start(const char* type, const char* host, const char* port);
void net_cipsend(const char* data, uint16_t len);
