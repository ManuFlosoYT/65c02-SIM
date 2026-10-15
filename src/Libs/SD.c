#include "SD.h"

static FATFS sd_fs;

int sd_mount(void) {
    return (int)f_mount(&sd_fs, "", 1);
}

extern const char* os_get_msg(uint8_t msg_id);

const char* sd_error_string(int res) {
    if (res == FR_OK) {
        return "OK";
    }
    return os_get_msg((uint8_t)res);
}

void sd_unmount(void) {
    f_unmount("");
}

bool sd_is_present(void) {
    return (SD_CTRL & SD_STAT_MOUNTED) != 0U;
}

uint8_t sd_open(SD_FILE* fp, const char* path, uint8_t mode) {
    return (f_open(fp, path, mode) == FR_OK) ? 1U : 0U;
}

uint8_t sd_close(SD_FILE* fp) {
    return (f_close(fp) == FR_OK) ? 1U : 0U;
}

int16_t sd_read(SD_FILE* fp, void* buf, uint16_t len) {
    UINT br = 0;
    if (f_read(fp, buf, len, &br) != FR_OK) {
        return -1;
    }
    return (int16_t)br;
}

int16_t sd_write(SD_FILE* fp, const void* buf, uint16_t len) {
    UINT bw = 0;
    if (f_write(fp, buf, len, &bw) != FR_OK) {
        return -1;
    }
    return (int16_t)bw;
}

uint8_t sd_puts(SD_FILE* fp, const char* str) {
    UINT bw = 0;
    uint16_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    if (f_write(fp, str, len, &bw) != FR_OK || bw != len) {
        return 0U;
    }
    return 1U;
}

int16_t sd_getc(SD_FILE* fp) {
    uint8_t c = 0;
    UINT br = 0;
    if (f_read(fp, &c, 1, &br) != FR_OK || br == 0) {
        return -1;
    }
    return (int16_t)c;
}

int sd_seek(SD_FILE* fp, uint32_t pos) {
    return (f_lseek(fp, pos) == FR_OK) ? 1 : 0;
}

uint32_t sd_tell(SD_FILE* fp) {
    return (uint32_t)f_tell(fp);
}

uint32_t sd_size(SD_FILE* fp) {
    return (uint32_t)f_size(fp);
}

int sd_sync(SD_FILE* fp) {
    return (f_sync(fp) == FR_OK) ? 1 : 0;
}

int sd_opendir(SD_DIR* dp, const char* path) {
    return (f_opendir(dp, path) == FR_OK) ? 1 : 0;
}

int sd_readdir(SD_DIR* dp, SD_INFO* fno) {
    if (f_readdir(dp, fno) != FR_OK) {
        return 0;
    }
    return (fno->fname[0] != '\0') ? 1 : 0;
}

void sd_closedir(SD_DIR* dp) {
    f_closedir(dp);
}

uint8_t sd_exists(const char* path) {
    FILINFO fno;
    return (f_stat(path, &fno) == FR_OK) ? 1U : 0U;
}

uint8_t sd_remove(const char* path) {
    return (f_unlink(path) == FR_OK) ? 1U : 0U;
}

uint8_t sd_rename(const char* old_path, const char* new_path) {
    return (f_rename(old_path, new_path) == FR_OK) ? 1U : 0U;
}

uint8_t sd_mkdir(const char* path) {
    return (f_mkdir(path) == FR_OK) ? 1U : 0U;
}

uint8_t sd_chdir(const char* path) {
    return (f_chdir(path) == FR_OK) ? 1U : 0U;
}

uint8_t sd_getcwd(char* buf, uint16_t len) {
    return (f_getcwd(buf, len) == FR_OK) ? 1U : 0U;
}
