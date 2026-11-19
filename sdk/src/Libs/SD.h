#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "fatfs/ff.h"

#if defined(MICRODOS) || defined(__MICRODOS__)
#define SD_CTRL (*(volatile uint8_t*)0x7FC8)
#define SD_DATA (*(volatile uint8_t*)0x7FC9)
#else
#define SD_CTRL (*(volatile uint8_t*)0x5008)
#define SD_DATA (*(volatile uint8_t*)0x5009)
#endif

#define SD_CTRL_CS      0x01U
#define SD_STAT_MOUNTED 0x01U

#define SD_READ          FA_READ
#define SD_WRITE         FA_WRITE
#define SD_CREATE_NEW    FA_CREATE_NEW
#define SD_CREATE_ALWAYS FA_CREATE_ALWAYS
#define SD_OPEN_ALWAYS   FA_OPEN_ALWAYS
#define SD_OPEN_APPEND   FA_OPEN_APPEND

typedef FIL SD_FILE;
typedef DIR SD_DIR;
typedef FILINFO SD_INFO;

int sd_mount(void);
const char* sd_error_string(int res);
void sd_unmount(void);
bool sd_is_present(void);
void sd_set_crc(uint8_t enable);

uint8_t  sd_open(SD_FILE* fp, const char* path, uint8_t mode);
uint8_t  sd_close(SD_FILE* fp);
int16_t  sd_read(SD_FILE* fp, void* buf, uint16_t len);
int16_t  sd_write(SD_FILE* fp, const void* buf, uint16_t len);
uint8_t  sd_puts(SD_FILE* fp, const char* str);
int16_t  sd_getc(SD_FILE* fp);
int      sd_seek(SD_FILE* fp, uint32_t pos);
uint32_t sd_tell(SD_FILE* fp);
uint32_t sd_size(SD_FILE* fp);
int      sd_sync(SD_FILE* fp);

int      sd_opendir(SD_DIR* dp, const char* path);
int      sd_readdir(SD_DIR* dp, SD_INFO* fno);
void     sd_closedir(SD_DIR* dp);

uint8_t  sd_exists(const char* path);
uint8_t  sd_remove(const char* path);
uint8_t  sd_rename(const char* old_path, const char* new_path);
uint8_t  sd_mkdir(const char* path);
uint8_t  sd_chdir(const char* path);
uint8_t  sd_getcwd(char* buf, uint16_t len);
