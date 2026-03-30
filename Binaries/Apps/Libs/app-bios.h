/*
 * app-bios.h — BIOS/OS API for microDOS .app binaries
 *
 * Functions are resolved by the app linker (generate_app_cfg.py) directly
 * to the fixed jump table addresses in ROM. Apps call these as normal C
 * functions; the generated JSR goes straight to the jump table entry.
 *
 * JUMPTABLE layout (see Linker/bios.s):
 *  $FF90 print_str   $FF93 println    $FF96 print_num   $FF99 print_hex_byte
 *  $FF9C bios_getchar $FF9F bios_putchar
 *  $FFA2 sd_open     $FFA5 sd_read    $FFA8 sd_write    $FFAB sd_close
 *  $FFAE sd_puts     $FFB1 sd_getc    $FFB4 sd_getcwd   $FFB7 sd_chdir
 *  $FFBA net_send    $FFBD net_cmd    $FFC0 net_send_num
 */
#ifndef APP_BIOS_H
#define APP_BIOS_H

#include <stdint.h>

/* Opaque handles for FatFs (sizes for TINY=1 config) */
typedef struct { uint8_t reserved[34]; } SD_FILE;
typedef struct { uint8_t reserved[40]; } SD_DIR;
typedef struct { uint8_t reserved[22]; } SD_INFO;
#define SD_READ          0x01
#define SD_WRITE         0x02
#define SD_CREATE_NEW    0x04
#define SD_CREATE_ALWAYS 0x08
#define SD_OPEN_ALWAYS   0x10

/* ESP8266 MMIO */
#define ESP8266_DATA   (*(volatile uint8_t*)0x5004)
#define ESP8266_STATUS (*(volatile uint8_t*)0x5005)
#define net_has_data() (ESP8266_STATUS & 0x80)
#define net_putc(c)    (ESP8266_DATA = (c))
#define net_getc()     (ESP8266_DATA)

/* BIOS — resolved to jump table by app linker */
void print_str(const char* s);
void println(const char* s);
void print_num(unsigned int n);
void print_hex_byte(unsigned char v);
char bios_getchar(void);
void bios_putchar(char c);

/* SD */
int sd_open(SD_FILE* fp, const char* path, uint8_t mode);
int sd_read(SD_FILE* fp, void* buf, uint16_t len);
int sd_write(SD_FILE* fp, const void* buf, uint16_t len);
int sd_close(SD_FILE* fp);
int sd_puts(SD_FILE* fp, const char* s);
int sd_getc(SD_FILE* fp);
int sd_getcwd(char* buf, uint16_t len);
int sd_chdir(const char* path);

/* NET */
void net_send(const char* s);
void net_cmd(const char* s);
void net_send_num(uint16_t n);

#endif /* APP_BIOS_H */
