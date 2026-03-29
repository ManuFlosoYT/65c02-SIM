/*
 * wget.c — microDOS HTTP/HTTPS downloader app
 * Compile with: ./compile-bin.sh wget --microDOS
 */
#include <stdint.h>
#include <string.h>
#include "Libs/app-bios.h"

#define arg_count  (*(volatile uint8_t*)0x60)
#define _args_ptr  ((char**)(*(uint16_t*)0x61))

int main(void) {
    char protocol[6];
    char host[64];
    char path[128];
    char* url;
    char* p;
    char* h;
    SD_FILE fp;
    uint16_t bytes_to_read = 0;
    uint8_t in_packet = 0;
    uint8_t header_finished = 0;
    char chr;
    int state = 0;
    char** args = _args_ptr;

    if (arg_count < 3) {
        print_str("U:"); println("wget <p> <u>");
        return 1;
    }

    url = args[2];

    if (strncmp(url, "http://", 7) == 0) {
        strcpy(protocol, "TCP");
        p = url + 7;
    } else if (strncmp(url, "https://", 8) == 0) {
        strcpy(protocol, "SSL");
        p = url + 8;
    } else {
        strcpy(protocol, "TCP");
        p = url;
    }

    h = host;
    while (*p && *p != '/' && (h - host < 63)) *h++ = *p++;
    *h = '\0';

    if (*p == '/')  { strncpy(path, p, 127); path[127] = '\0'; }
    else            { strcpy(path, "/"); }

    if (host[0] == '\0') {
        print_str("E:"); println("host");
        return 1;
    }

    println(host);

    if (strcmp(protocol, "SSL") == 0) net_send("AT+CIPSTART=\"SSL\",\"");
    else                              net_send("AT+CIPSTART=\"TCP\",\"");
    net_send(host);
    if (strcmp(protocol, "SSL") == 0) net_cmd("\",443");
    else                              net_cmd("\",80");

    while (1) {
        if (net_has_data()) {
            chr = (char)net_getc();
            if (chr == 'C') break;
            if (chr == 'E') { print_str("E:"); println("conn"); return 1; }
        }
    }

    {
        uint16_t req_len = 4 + (uint16_t)strlen(path) + 11 + 6 + (uint16_t)strlen(host) + 21 + 2;
        net_send("AT+CIPSEND=");
        net_send_num(req_len);
        net_send("\r\n");
        while (1) {
            if (net_has_data()) {
                chr = (char)net_getc();
                if (chr == '>') break;
                if (chr == 'E') break;
            }
        }
        net_send("GET "); net_send(path); net_send(" HTTP/1.0\r\n");
        net_send("Host: "); net_send(host); net_send("\r\n");
        net_send("Connection: close\r\n\r\n");
    }

    if (!sd_open(&fp, args[1], SD_WRITE | SD_CREATE_ALWAYS)) {
        print_str("E:"); println("file");
        return 1;
    }

    println("Wait");

    while (1) {
        if (net_has_data()) {
            chr = (char)net_getc();

            if (!in_packet && chr == '+') {
                if (net_getc() == 'I' && net_getc() == 'P' &&
                    net_getc() == 'D' && net_getc() == ',') {
                    bytes_to_read = 0;
                    while (1) {
                        chr = (char)net_getc();
                        if (chr == ':') break;
                        if (chr >= '0' && chr <= '9')
                            bytes_to_read = bytes_to_read * 10 + (chr - '0');
                    }
                    in_packet = 1;
                    continue;
                }
            }

            if (in_packet) {
                if (header_finished) {
                    sd_write(&fp, &chr, 1);
                } else {
                    if      (state == 0 && chr == '\r') state = 1;
                    else if (state == 1 && chr == '\n') state = 2;
                    else if (state == 2 && chr == '\r') state = 3;
                    else if (state == 3 && chr == '\n') header_finished = 1;
                    else { state = 0; if (chr == '\r') state = 1; }
                }
                bytes_to_read--;
                if (bytes_to_read == 0) in_packet = 0;
                continue;
            }

            if (chr == 'O' && net_getc() == 'K') break;
            if (chr == 'R' && net_getc() == 'R') break;
        }
    }

    sd_close(&fp);
    println("Done.");
    return 0;
}
