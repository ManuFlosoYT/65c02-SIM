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

void net_send(const char* s);
void net_send_num(uint16_t n);
void net_cmd(const char* s);
void net_wifi(const char* ssid, const char* pass);
void net_start(const char* type, const char* host, const char* port);
void net_cipsend(const char* data, uint16_t len);

#endif
