/*-----------------------------------------------------------------------*/
/* Low level disk I/O for FatFs on the 65C02 simulator                   */
/* SPI SD card via 6522 VIA Port B                                       */
/*                                                                       */
/*  Port B pin mapping:                                                  */
/*    PB0 = MOSI  (output)                                               */
/*    PB1 = MISO  (input)                                                */
/*    PB2 = CLK   (output)                                               */
/*    PB3 = /CS   (output)  active low                                   */
/*-----------------------------------------------------------------------*/

#include "diskio.h"

#include <stdint.h>

/* 6522 VIA register addresses (match VIA.h) */
#define VIA_PORTB (*(volatile uint8_t*)0x6000)
#define VIA_DDRB (*(volatile uint8_t*)0x6002)

/* SPI pin masks on Port B */
#define SPI_MOSI 0x01u /* PB0 */
#define SPI_MISO 0x02u /* PB1 */
#define SPI_CLK 0x04u  /* PB2 */
#define SPI_CS_N 0x08u /* PB3 */

/* Direction bits: MOSI/CLK/CS are outputs, MISO is input */
#define SPI_DDR_MASK (SPI_MOSI | SPI_CLK | SPI_CS_N)

/* -----------------------------------------------------------------------
   Internal helpers
   ----------------------------------------------------------------------- */

static DSTATUS Stat = STA_NOINIT;

static void spi_init(void) {
    /* Set MOSI, CLK, CS as outputs; MISO as input */
    VIA_DDRB = (VIA_DDRB & ~SPI_MISO) | SPI_DDR_MASK;
    /* Deassert CS, CLK low, MOSI low */
    VIA_PORTB = (VIA_PORTB & ~(SPI_MOSI | SPI_CLK)) | SPI_CS_N;
}

/* Send/receive one byte MSB-first, SPI mode 0 (bit-bang) */
static uint8_t spi_xfer(uint8_t mosi) {
    uint8_t miso = 0;
    int i;
    for (i = 7; i >= 0; i--) {
        /* Drive MOSI */
        if (mosi & (uint8_t)(1u << i)) {
            VIA_PORTB |= SPI_MOSI;
        } else {
            VIA_PORTB &= ~SPI_MOSI;
        }
        /* Rising edge – card samples MOSI */
        VIA_PORTB |= SPI_CLK;
        /* Sample MISO */
        if (VIA_PORTB & SPI_MISO) {
            miso |= (uint8_t)(1u << i);
        }
        /* Falling edge */
        VIA_PORTB &= ~SPI_CLK;
    }
    return miso;
}

static void spi_cs_low(void) { VIA_PORTB &= ~SPI_CS_N; }
static void spi_cs_high(void) { VIA_PORTB |= SPI_CS_N; }

/* Send SD command, return R1 response byte */
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg) {
    uint8_t crc = 0x01;
    uint8_t r;
    int i;
    if (cmd == 0) crc = 0x95;
    if (cmd == 8) crc = 0x87;

    spi_xfer((uint8_t)(0x40u | cmd));
    spi_xfer((uint8_t)((arg >> 24) & 0xFFu));
    spi_xfer((uint8_t)((arg >> 16) & 0xFFu));
    spi_xfer((uint8_t)((arg >> 8) & 0xFFu));
    spi_xfer((uint8_t)(arg & 0xFFu));
    spi_xfer(crc);

    /* Wait up to 8 bytes for response */
    r = 0xFF;
    for (i = 0; i < 8; i++) {
        r = spi_xfer(0xFF);
        if (!(r & 0x80u)) break;
    }
    return r;
}

/* Wait until card is not busy */
static int sd_wait_ready(void) {
    int i;
    for (i = 0; i < 5000; i++) {
        if (spi_xfer(0xFF) == 0xFF) return 1;
    }
    return 0;
}

/* Read 512-byte data block from card */
static int sd_recv_block(BYTE* buf, UINT len) {
    uint8_t token;
    UINT i;
    /* Wait for data token 0xFE */
    token = 0xFF;
    for (i = 0; i < 2000u; i++) {
        token = spi_xfer(0xFF);
        if (token != 0xFF) break;
    }
    if (token != 0xFE) return 0;

    for (i = 0; i < len; i++) {
        buf[i] = spi_xfer(0xFF);
    }
    spi_xfer(0xFF); /* Discard CRC */
    spi_xfer(0xFF);
    return 1;
}

/* Write 512-byte data block to card */
static int sd_send_block(const BYTE* buf, uint8_t token) {
    UINT i;
    uint8_t resp;
    if (!sd_wait_ready()) return 0;

    spi_xfer(token);
    for (i = 0; i < 512u; i++) {
        spi_xfer(buf[i]);
    }
    spi_xfer(0xFF); /* Dummy CRC */
    spi_xfer(0xFF);

    resp = spi_xfer(0xFF) & 0x1Fu;
    return (resp == 0x05); /* Data accepted */
}

/* Initialise SD card (CMD0 -> CMD8 -> ACMD41) */
static int sd_card_init(void) {
    uint8_t r;
    int i;

    /* >=74 clock pulses with CS high */
    spi_cs_high();
    for (i = 0; i < 10; i++) spi_xfer(0xFF);

    /* CMD0 – go idle */
    spi_cs_low();
    if (sd_send_cmd(0, 0) != 0x01) {
        spi_cs_high();
        return 1;
    }

    /* CMD8 – check voltage (SD v2) */
    r = sd_send_cmd(8, 0x000001AAul);
    if (r == 0x01) {
        spi_xfer(0xFF);
        spi_xfer(0xFF);
        spi_xfer(0xFF);
        spi_xfer(0xFF);
    }

    /* ACMD41 – initialise */
    for (i = 0; i < 2000; i++) {
        spi_cs_low();
        sd_send_cmd(55, 0);                /* CMD55 */
        r = sd_send_cmd(41, 0x40000000ul); /* ACMD41 with HCS */
        spi_cs_high();
        spi_xfer(0xFF);
        if (r == 0x00) return 0; /* Card ready */
        spi_cs_low();
    }

    spi_cs_high();
    return 1;
}

/* ======================================================================
   FatFs required functions
   ====================================================================== */

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;
    spi_init();
    if (sd_card_init() == 0) {
        Stat &= ~STA_NOINIT;
    } else {
        Stat = STA_NOINIT;
    }
    return Stat;
}

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;
    return Stat;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    UINT i;
    if (pdrv != 0 || (Stat & STA_NOINIT)) return RES_NOTRDY;
    if (count == 0) return RES_PARERR;

    for (i = 0; i < count; i++) {
        spi_cs_low();
        if (sd_send_cmd(17, (uint32_t)(sector + i)) == 0x00) {
            if (sd_recv_block(buff, 512u)) {
                buff += 512;
            }
        }
        spi_cs_high();
        spi_xfer(0xFF);
    }
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    UINT i;
    if (pdrv != 0 || (Stat & STA_NOINIT)) return RES_NOTRDY;
    if (Stat & STA_PROTECT) return RES_WRPRT;
    if (count == 0) return RES_PARERR;

    for (i = 0; i < count; i++) {
        spi_cs_low();
        if (sd_send_cmd(24, (uint32_t)(sector + i)) == 0x00) {
            if (sd_send_block(buff, 0xFE)) {
                buff += 512;
            }
        }
        spi_cs_high();
        spi_xfer(0xFF);
    }
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    if (pdrv != 0 || (Stat & STA_NOINIT)) return RES_NOTRDY;

    switch (cmd) {
        case CTRL_SYNC:
            spi_cs_low();
            sd_wait_ready();
            spi_cs_high();
            return RES_OK;
        case GET_SECTOR_SIZE:
            *(WORD*)buff = 512;
            return RES_OK;
        case GET_BLOCK_SIZE:
            *(DWORD*)buff = 1;
            return RES_OK;
        default:
            return RES_PARERR;
    }
}
