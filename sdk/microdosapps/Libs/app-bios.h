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
uint8_t  sd_open(SD_FILE* fp, const char* path, uint8_t mode);
int16_t  sd_read(SD_FILE* fp, void* buf, uint16_t len);
int16_t  sd_write(SD_FILE* fp, const void* buf, uint16_t len);
uint8_t  sd_close(SD_FILE* fp);
uint8_t  sd_puts(SD_FILE* fp, const char* s);
int16_t  sd_getc(SD_FILE* fp);
uint8_t  sd_getcwd(char* buf, uint16_t len);
uint8_t  sd_chdir(const char* path);

/* NET */
void net_send(const char* s);
void net_cmd(const char* s);
void net_send_num(uint16_t n);

/* SID BIOS API */
void sid_write(uint8_t reg, uint8_t val);
void sid_reset(void);

/* Virtual Paged Memory */
void os_load_app_page(uint8_t page_id);

// SID Register Offsets
#define FREQ_LO_1 0x00
#define FREQ_HI_1 0x01
#define PW_LO_1   0x02
#define PW_HI_1   0x03
#define CTRL_1    0x04
#define AD_1      0x05
#define SR_1      0x06

#define FREQ_LO_2 0x07
#define FREQ_HI_2 0x08
#define PW_LO_2   0x09
#define PW_HI_2   0x0A
#define CTRL_2    0x0B
#define AD_2      0x0C
#define SR_2      0x0D

#define FREQ_LO_3 0x0E
#define FREQ_HI_3 0x0F
#define PW_LO_3   0x10
#define PW_HI_3   0x11
#define CTRL_3    0x12
#define AD_3      0x13
#define SR_3      0x14

#define FC_LO     0x15
#define FC_HI     0x16
#define RES_FILT  0x17
#define MODE_VOL  0x18

// Waveforms
#define WAVE_GATE  0x01
#define WAVE_TRI   0x10
#define WAVE_SAW   0x20
#define WAVE_PULSE 0x40
#define WAVE_NOISE 0x80

// Common Notes
#define NOTE_C3  0x0892
#define NOTE_Cs3 0x0914
#define NOTE_D3  0x099E
#define NOTE_Ds3 0x0A32
#define NOTE_E3  0x0ACC
#define NOTE_F3  0x0B71
#define NOTE_Fs3 0x0C1F
#define NOTE_G3  0x0CD8
#define NOTE_Gs3 0x0D9C
#define NOTE_A3  0x0E6A
#define NOTE_As3 0x0F47
#define NOTE_B3  0x102F
#define NOTE_C4  0x1124
#define NOTE_E4  0x1599
#define NOTE_G4  0x19B0
#define NOTE_C5  0x2249

#endif /* APP_BIOS_H */
