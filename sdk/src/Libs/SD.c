/*-----------------------------------------------------------------------*/
/* SD.c – FatFs wrapper implementation for the 65C02 simulator           */
/*-----------------------------------------------------------------------*/

#include "SD.h"

static FATFS _sd_fs;

/* -----------------------------------------------------------------------
   Volume
   ----------------------------------------------------------------------- */

int sd_mount(void) { return (int)f_mount(&_sd_fs, "", 1); }

const char* sd_error_string(int res) {
    switch (res) {
        case FR_OK: return "OK";
        case FR_DISK_ERR: return "Disk Error";
        case FR_INT_ERR: return "Internal Error";
        case FR_NOT_READY: return "Not Ready";
        case FR_NO_FILE: return "File Not Found";
        case FR_NO_PATH: return "Path Not Found";
        case FR_INVALID_NAME: return "Invalid Name";
        case FR_DENIED: return "Access Denied";
        case FR_EXIST: return "Already Exists";
        case FR_INVALID_OBJECT: return "Invalid Object";
        case FR_WRITE_PROTECTED: return "Write Protected";
        case FR_INVALID_DRIVE: return "Invalid Drive";
        case FR_NOT_ENABLED: return "Volume Not Enabled";
        case FR_NO_FILESYSTEM: return "No Filesystem";
        case FR_MKFS_ABORTED: return "Mkfs Aborted";
        case FR_TIMEOUT: return "Timeout";
        case FR_LOCKED: return "Locked";
        case FR_NOT_ENOUGH_CORE: return "Out of Memory";
        case FR_TOO_MANY_OPEN_FILES: return "Too Many Open Files";
        case FR_INVALID_PARAMETER: return "Invalid Parameter";
        default: return "Unknown Error";
    }
}

void sd_unmount(void) { f_unmount(""); }

int sd_is_present(void) { return (SD_CTRL & SD_STAT_MOUNTED); }

/* -----------------------------------------------------------------------
   File I/O
   ----------------------------------------------------------------------- */

uint8_t sd_open(SD_FILE* fp, const char* path, uint8_t mode) { return (f_open(fp, path, mode) == FR_OK); }

uint8_t sd_close(SD_FILE* fp) { return (f_close(fp) == FR_OK); }

int16_t sd_read(SD_FILE* fp, void* buf, uint16_t len) {
    UINT br;
    if (f_read(fp, buf, len, &br) != FR_OK) return -1;
    return (int16_t)br;
}

int16_t sd_write(SD_FILE* fp, const void* buf, uint16_t len) {
    UINT bw;
    if (f_write(fp, buf, len, &bw) != FR_OK) return -1;
    return (int16_t)bw;
}

uint8_t sd_puts(SD_FILE* fp, const char* str) {
    UINT bw;
    uint16_t len = 0;
    while (str[len]) len++;
    if (f_write(fp, str, len, &bw) != FR_OK || bw != len) return 0;
    return 1;
}

int16_t sd_getc(SD_FILE* fp) {
    uint8_t c;
    UINT br;
    if (f_read(fp, &c, 1, &br) != FR_OK || br == 0) return -1;
    return (int16_t)c;
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

uint8_t sd_exists(const char* path) {
    FILINFO fno;
    return (f_stat(path, &fno) == FR_OK);
}

uint8_t sd_remove(const char* path) { return (f_unlink(path) == FR_OK); }

uint8_t sd_rename(const char* old_path, const char* new_path) { return (f_rename(old_path, new_path) == FR_OK); }

uint8_t sd_mkdir(const char* path) { return (f_mkdir(path) == FR_OK); }

uint8_t sd_chdir(const char* path) { return (f_chdir(path) == FR_OK); }
uint8_t sd_getcwd(char* buf, uint16_t len) { return (f_getcwd(buf, len) == FR_OK); }
