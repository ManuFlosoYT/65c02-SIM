/*-----------------------------------------------------------------------*/
/* SD.c – FatFs wrapper implementation for the 65C02 simulator           */
/*-----------------------------------------------------------------------*/

#include "SD.h"

static FATFS _sd_fs;

/* -----------------------------------------------------------------------
   Volume
   ----------------------------------------------------------------------- */

int sd_mount(void) { return (f_mount(&_sd_fs, "", 1) == FR_OK); }

void sd_unmount(void) { f_unmount(""); }

/* -----------------------------------------------------------------------
   File I/O
   ----------------------------------------------------------------------- */

int sd_open(SD_FILE* fp, const char* path, uint8_t mode) { return (f_open(fp, path, mode) == FR_OK); }

int sd_close(SD_FILE* fp) { return (f_close(fp) == FR_OK); }

int sd_read(SD_FILE* fp, void* buf, uint16_t len) {
    UINT br;
    if (f_read(fp, buf, len, &br) != FR_OK) return -1;
    return (int)br;
}

int sd_write(SD_FILE* fp, const void* buf, uint16_t len) {
    UINT bw;
    if (f_write(fp, buf, len, &bw) != FR_OK) return -1;
    return (int)bw;
}

int sd_puts(SD_FILE* fp, const char* str) {
    while (*str) {
        UINT bw;
        if (f_write(fp, str, 1, &bw) != FR_OK || bw == 0) return 0;
        str++;
    }
    return 1;
}

int sd_getc(SD_FILE* fp) {
    uint8_t c;
    UINT br;
    if (f_read(fp, &c, 1, &br) != FR_OK || br == 0) return -1;
    return (int)c;
}

int sd_seek(SD_FILE* fp, uint32_t pos) { return (f_lseek(fp, pos) == FR_OK); }

uint32_t sd_tell(SD_FILE* fp) { return (uint32_t)f_tell(fp); }

uint32_t sd_size(SD_FILE* fp) { return (uint32_t)f_size(fp); }

int sd_sync(SD_FILE* fp) { return (f_sync(fp) == FR_OK); }

/* -----------------------------------------------------------------------
   Directory
   ----------------------------------------------------------------------- */

int sd_opendir(SD_DIR* dp, const char* path) { return (f_opendir(dp, path) == FR_OK); }

int sd_readdir(SD_DIR* dp, SD_INFO* fno) {
    if (f_readdir(dp, fno) != FR_OK) return 0;
    return (fno->fname[0] != '\0');
}

void sd_closedir(SD_DIR* dp) { f_closedir(dp); }

/* -----------------------------------------------------------------------
   Filesystem utilities
   ----------------------------------------------------------------------- */

int sd_exists(const char* path) {
    FILINFO fno;
    return (f_stat(path, &fno) == FR_OK);
}

int sd_remove(const char* path) { return (f_unlink(path) == FR_OK); }

int sd_rename(const char* old_path, const char* new_path) { return (f_rename(old_path, new_path) == FR_OK); }

int sd_mkdir(const char* path) { return (f_mkdir(path) == FR_OK); }

int sd_chdir(const char* path) { return (f_chdir(path) == FR_OK); }
