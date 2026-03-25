/*-----------------------------------------------------------------------*/
/* SD.h – Simple high-level API over FatFs for the 65C02 simulator       */
/*                                                                       */
/*  Include "Libs/SD.h" in your program and add "Libs/SD.c" to the      */
/*  compilation. FatFs (ff.c + diskio.c) is compiled automatically by   */
/*  compile-bin.sh when SD.h is detected.                               */
/*-----------------------------------------------------------------------*/

#ifndef SD_H
#define SD_H

#include <stdint.h>

#include "fatfs/ff.h"

/* MMIO Registers */
#define SD_CTRL (*(volatile uint8_t*)0x5008)
#define SD_DATA (*(volatile uint8_t*)0x5009)

/* Register bits */
#define SD_CTRL_CS      0x01  /* Bit 0: Chip Select (Write: 1=Active/Low, 0=Inactive/High) */
#define SD_STAT_MOUNTED 0x01  /* Bit 0: Mounted Status (Read: 1=Mounted, 0=None) */

/* Friendly open-mode aliases */
#define SD_READ FA_READ
#define SD_WRITE FA_WRITE
#define SD_CREATE_NEW FA_CREATE_NEW
#define SD_CREATE_ALWAYS FA_CREATE_ALWAYS
#define SD_OPEN_ALWAYS FA_OPEN_ALWAYS
#define SD_OPEN_APPEND FA_OPEN_APPEND

typedef FIL SD_FILE;
typedef DIR SD_DIR;
typedef FILINFO SD_INFO;

/* Volume */
int sd_mount(void);
const char* sd_error_string(int res);
void sd_unmount(void);
int sd_is_present(void);

/* File I/O */
int sd_open(SD_FILE* fp, const char* path, uint8_t mode);
int sd_close(SD_FILE* fp);
int sd_read(SD_FILE* fp, void* buf, uint16_t len);
int sd_write(SD_FILE* fp, const void* buf, uint16_t len);
int sd_puts(SD_FILE* fp, const char* str);
int sd_getc(SD_FILE* fp);
int sd_seek(SD_FILE* fp, uint32_t pos);
uint32_t sd_tell(SD_FILE* fp);
uint32_t sd_size(SD_FILE* fp);
int sd_sync(SD_FILE* fp);

/* Directory */
int sd_opendir(SD_DIR* dp, const char* path);
int sd_readdir(SD_DIR* dp, SD_INFO* fno);
void sd_closedir(SD_DIR* dp);

/* Filesystem */
int sd_exists(const char* path);
int sd_remove(const char* path);
int sd_rename(const char* old_path, const char* new_path);
int sd_mkdir(const char* path);
int sd_chdir(const char* path);
int sd_getcwd(char* buf, uint16_t len);

#endif /* SD_H */
