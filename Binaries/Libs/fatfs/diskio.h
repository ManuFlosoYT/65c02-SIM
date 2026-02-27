/*-----------------------------------------------------------------------/
/  Low level disk interface modlue include file   (C)ChaN, 2019          /
/-----------------------------------------------------------------------*/

#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "ff.h"

/* Status of Disk Functions */
typedef BYTE DSTATUS;

/* Results of Disk Functions */
typedef enum {
    RES_OK = 0, /* 0: Successful */
    RES_ERROR,  /* 1: R/W Error */
    RES_WRPRT,  /* 2: Write Protected */
    RES_NOTRDY, /* 3: Not Ready */
    RES_PARERR  /* 4: Invalid Parameter */
} DRESULT;

/* Disk Status Bits (DSTATUS) */
#define STA_NOINIT 0x01  /* Drive not initialized */
#define STA_NODISK 0x02  /* No medium in the drive */
#define STA_PROTECT 0x04 /* Write protected */

/* Command code for disk_ioctrl fucntion */
#define CTRL_SYNC 0        /* Complete pending write process */
#define GET_SECTOR_COUNT 1 /* Get media size */
#define GET_SECTOR_SIZE 2  /* Get sector size */
#define GET_BLOCK_SIZE 3   /* Get erase block size */
#define CTRL_TRIM 4        /* Inform device that the data on the block of sectors is no longer used */

/* Generic command (Not used by FatFs) */
#define CTRL_POWER 5  /* Get/Set power status */
#define CTRL_LOCK 6   /* Lock/Unlock media removal */
#define CTRL_EJECT 7  /* Eject media */
#define CTRL_FORMAT 8 /* Create physical format on the media */

/* MMC/SDC specific ioctl command */
#define MMC_GET_TYPE 10
#define MMC_GET_CSD 11
#define MMC_GET_CID 12
#define MMC_GET_OCR 13
#define MMC_GET_SDSTAT 14
#define ISDIO_READ 55
#define ISDIO_WRITE 56
#define ISDIO_MRITE 57

DSTATUS disk_initialize(BYTE pdrv);
DSTATUS disk_status(BYTE pdrv);
DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count);
DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count);
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff);

#ifdef __cplusplus
}
#endif

#endif
