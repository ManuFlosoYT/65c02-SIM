/*
 * app-bios.h — BIOS/OS API for microDOS .app binaries
 *
 * Functions are resolved by the app linker (generate_app_cfg.py) directly
 * to the fixed jump table addresses in ROM. Apps call these as normal C
 * functions; the generated JSR goes straight to the jump table entry.
 *
 * JUMPTABLE layout (see Linker/bios.s):
 */
#ifndef APP_BIOS_H
#define APP_BIOS_H

#include <stdbool.h>
#include <stdint.h>

/* ========================================================================= */
/*                              APP MEMORY                                   */
/* ========================================================================= */

enum {
    APP_SAFE_RAM_START  = 0x0200,
    APP_SAFE_RAM_END    = 0x53FF,
    APP_EXTRA_RAM_START = 0x5FC0,
    APP_EXTRA_RAM_END   = 0x7FBF,
    APP_EXTRA_RAM_SIZE  = 0x2000
};

/* ========================================================================= */
/*                           APP ARGUMENTS                                   */
/* ========================================================================= */

#define arg_count  (*(volatile uint8_t*)0x60)
#define _args_ptr  ((char**)(*(uint16_t*)0x61))


/* ========================================================================= */
/*                             SYSTEM BIOS                                   */
/* ========================================================================= */

/* BIOS — resolved to jump table by app linker */
void print_str(const char* s);
void println(const char* s);
void print_num(unsigned int n);
void print_hex_byte(unsigned char v);
char bios_getchar(void);
char bios_getchar_nb(void);
void bios_putchar(char c);


/* ========================================================================= */
/*                         FILE SYSTEM (SD/FATFS)                            */
/* ========================================================================= */

/* Opaque handles for FatFs (sizes for TINY=1 config) */
typedef struct { uint8_t reserved[40]; } SD_FILE;
typedef struct { uint8_t reserved[40]; } SD_DIR;
typedef struct {
    uint32_t fsize;
    uint16_t fdate;
    uint16_t ftime;
    uint8_t  fattrib;
    char     fname[13];
} SD_INFO;

enum {
    AM_DIR           = 0x10
};

enum {
    SD_READ          = 0x01,
    SD_WRITE         = 0x02,
    SD_CREATE_NEW    = 0x04,
    SD_CREATE_ALWAYS = 0x08,
    SD_OPEN_ALWAYS   = 0x10
};

uint8_t  sd_open(SD_FILE* fp, const char* path, uint8_t mode);
int16_t  sd_read(SD_FILE* fp, void* buf, uint16_t len);
int16_t  sd_write(SD_FILE* fp, const void* buf, uint16_t len);
uint8_t  sd_close(SD_FILE* fp);
uint8_t  sd_puts(SD_FILE* fp, const char* s);
int16_t  sd_getc(SD_FILE* fp);
uint8_t  sd_getcwd(char* buf, uint16_t len);
uint8_t  sd_chdir(const char* path);
int      sd_opendir(SD_DIR* dp, const char* path);
int      sd_readdir(SD_DIR* dp, SD_INFO* fno);
void     sd_closedir(SD_DIR* dp);
uint8_t  sd_mkdir(const char* path);
uint8_t  sd_remove(const char* path);
uint8_t  sd_exists(const char* path);
int      sd_seek(SD_FILE* fp, uint32_t pos);


/* ========================================================================= */
/*                         NETWORK & WIFI (ESP8266)                          */
/* ========================================================================= */

/* ESP8266 MMIO */
#define ESP8266_DATA   (*(volatile uint8_t*)0x7FC4)
#define ESP8266_STATUS (*(volatile uint8_t*)0x7FC5)
#define net_has_data() (ESP8266_STATUS & 0x80)
#define net_putc(c)    (ESP8266_DATA = (c))
#define net_getc()     (ESP8266_DATA)

/* NET API */
void net_send(const char* s);
void net_cmd(const char* s);
void net_send_num(uint16_t n);
void net_wifi(const char* ssid, const char* pass);


/* ========================================================================= */
/*                          HARDWARE IO (VIA 6522)                           */
/* ========================================================================= */

/* VIA 6522 MMIO */
#define VIA_ORB    (*(volatile uint8_t*)0x7FD0)
#define VIA_IRB    (*(volatile uint8_t*)0x7FD0)
#define VIA_ORA    (*(volatile uint8_t*)0x7FD1)
#define VIA_IRA    (*(volatile uint8_t*)0x7FD1)
#define VIA_DDRB   (*(volatile uint8_t*)0x7FD2)
#define VIA_DDRA   (*(volatile uint8_t*)0x7FD3)
#define VIA_T1C_L  (*(volatile uint8_t*)0x7FD4)
#define VIA_T1C_H  (*(volatile uint8_t*)0x7FD5)
#define VIA_T1L_L  (*(volatile uint8_t*)0x7FD6)
#define VIA_T1L_H  (*(volatile uint8_t*)0x7FD7)
#define VIA_T2C_L  (*(volatile uint8_t*)0x7FD8)
#define VIA_T2C_H  (*(volatile uint8_t*)0x7FD9)
#define VIA_SR     (*(volatile uint8_t*)0x7FDA)
#define VIA_ACR    (*(volatile uint8_t*)0x7FDB)
#define VIA_PCR    (*(volatile uint8_t*)0x7FDC)
#define VIA_IFR    (*(volatile uint8_t*)0x7FDD)
#define VIA_IER    (*(volatile uint8_t*)0x7FDE)
#define VIA_ORA_NH (*(volatile uint8_t*)0x7FDF)
#define VIA_IRA_NH (*(volatile uint8_t*)0x7FDF)
#define PORTB      VIA_ORB
#define DDRB       VIA_DDRB


/* ========================================================================= */
/*                              AUDIO (SID)                                  */
/* ========================================================================= */

// SID Register Offsets
enum {
    FREQ_LO_1 = 0x00,
    FREQ_HI_1 = 0x01,
    PW_LO_1   = 0x02,
    PW_HI_1   = 0x03,
    CTRL_1    = 0x04,
    AD_1      = 0x05,
    SR_1      = 0x06,

    FREQ_LO_2 = 0x07,
    FREQ_HI_2 = 0x08,
    PW_LO_2   = 0x09,
    PW_HI_2   = 0x0A,
    CTRL_2    = 0x0B,
    AD_2      = 0x0C,
    SR_2      = 0x0D,

    FREQ_LO_3 = 0x0E,
    FREQ_HI_3 = 0x0F,
    PW_LO_3   = 0x10,
    PW_HI_3   = 0x11,
    CTRL_3    = 0x12,
    AD_3      = 0x13,
    SR_3      = 0x14,

    FC_LO     = 0x15,
    FC_HI     = 0x16,
    RES_FILT  = 0x17,
    MODE_VOL  = 0x18
};

// Waveforms & Features
enum {
    WAVE_GATE  = 0x01,
    WAVE_SYNC  = 0x02,
    WAVE_RING  = 0x04,
    WAVE_TEST  = 0x08,
    WAVE_TRI   = 0x10,
    WAVE_SAW   = 0x20,
    WAVE_PULSE = 0x40,
    WAVE_NOISE = 0x80,

    WAVE_TRI_SAW       = (WAVE_TRI | WAVE_SAW),
    WAVE_TRI_PULSE     = (WAVE_TRI | WAVE_PULSE),
    WAVE_SAW_PULSE     = (WAVE_SAW | WAVE_PULSE),
    WAVE_TRI_SAW_PULSE = (WAVE_TRI | WAVE_SAW | WAVE_PULSE)
};

// Common Notes
enum {
    NOTE_C3  = 0x0892,
    NOTE_Cs3 = 0x0914,
    NOTE_D3  = 0x099E,
    NOTE_Ds3 = 0x0A32,
    NOTE_E3  = 0x0ACC,
    NOTE_F3  = 0x0B71,
    NOTE_Fs3 = 0x0C1F,
    NOTE_G3  = 0x0CD8,
    NOTE_Gs3 = 0x0D9C,
    NOTE_A3  = 0x0E6A,
    NOTE_As3 = 0x0F47,
    NOTE_B3  = 0x102F,
    NOTE_C4  = 0x1124,
    NOTE_E4  = 0x1599,
    NOTE_G4  = 0x19B0,
    NOTE_C5  = 0x2249
};

/* SID BIOS API */
void sid_write(uint8_t reg, uint8_t val);
void sid_reset(void);

#define sid_trigger_note(voice, freq, ctrl) do { \
    uint8_t _offset = ((voice) - 1) * 7; \
    sid_write(_offset + 0x00, (freq) & 0xFF); \
    sid_write(_offset + 0x01, ((freq) >> 8) & 0xFF); \
    sid_write(_offset + 0x04, (ctrl)); \
} while(0)

#define sid_stop_note(voice) do { \
    uint8_t _offset = ((voice) - 1) * 7; \
    sid_write(_offset + 0x04, 0x00); \
} while(0)

#define sid_set_envelope(voice, attack, decay, sustain, release) do { \
    uint8_t _offset = ((voice) - 1) * 7; \
    sid_write(_offset + 0x05, ((attack) << 4) | ((decay) & 0x0F)); \
    sid_write(_offset + 0x06, ((sustain) << 4) | ((release) & 0x0F)); \
} while(0)

#define sid_set_pw(voice, pw) do { \
    uint8_t _offset = ((voice) - 1) * 7; \
    sid_write(_offset + 0x02, (pw) & 0xFF); \
    sid_write(_offset + 0x03, ((pw) >> 8) & 0x0F); \
} while(0)

#define sid_set_volume(vol) do { \
    sid_write(0x18, (vol) & 0x0F); \
} while(0)

#define sid_set_filter_cutoff(cutoff) do { \
    sid_write(0x15, (cutoff) & 0x07); \
    sid_write(0x16, ((cutoff) >> 3) & 0xFF); \
} while(0)

#define sid_set_filter_config(reson, voices, mode, vol) do { \
    sid_write(0x17, ((reson) << 4) | ((voices) & 0x0F)); \
    sid_write(0x18, (mode) | ((vol) & 0x0F)); \
} while(0)


/* ========================================================================= */
/*                         VIRTUAL PAGED MEMORY                              */
/* ========================================================================= */

void os_load_app_page(uint8_t page_id);

#define SWAP_WINDOW_ADDR ((void*)0x2800)
enum {
    SWAP_WINDOW_SIZE = 0x2000
};

uint8_t os_swap_save(const char* name, void* addr, uint16_t size);
uint8_t os_swap_load(const char* name, void* addr, uint16_t size);

void* os_alloc(uint16_t size);
void  os_free(void* ptr);

#endif /* APP_BIOS_H */
