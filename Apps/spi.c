/* spi.c - microDOS command */
#include "../Libs/app-bios.h"
#include <stdint.h>

#define SPI_PORT VIA_ORA
#define SPI_DDR  VIA_DDRA

#define SPI_CLK  0x01
#define SPI_MOSI 0x02
#define SPI_MISO 0x04
#define SPI_CS   0x08

static uint16_t parse_hex(const char *str) {
    uint16_t val = 0;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
    }
    while (*str) {
        val <<= 4;
        if (*str >= '0' && *str <= '9') {
            val |= (*str - '0');
        } else if (*str >= 'A' && *str <= 'F') {
            val |= (*str - 'A' + 10);
        } else if (*str >= 'a' && *str <= 'f') {
            val |= (*str - 'a' + 10);
        }
        str++;
    }
    return val;
}

static uint8_t spi_transfer(uint8_t data) {
    uint8_t i;
    uint8_t rx = 0;
    
    SPI_PORT &= ~SPI_CS; /* Select */
    
    for (i = 0; i < 8; i++) {
        if (data & 0x80) {
            SPI_PORT |= SPI_MOSI;
        } else {
            SPI_PORT &= ~SPI_MOSI;
        }
        
        SPI_PORT |= SPI_CLK; /* Rising edge */
        
        rx <<= 1;
        if (SPI_PORT & SPI_MISO) {
            rx |= 0x01;
        }
        
        SPI_PORT &= ~SPI_CLK; /* Falling edge */
        data <<= 1;
    }
    
    SPI_PORT |= SPI_CS; /* Deselect */
    
    return rx;
}

int main(void) {
    char **args = _args_ptr;
    uint8_t tx_val;
    uint8_t rx_val;
    
    if (arg_count < 2) {
        println("Usage: spi <hex_val>");
        println("Notes: Bit-bangs VIA_ORA. CLK=PA0, MOSI=PA1, MISO=PA2, CS=PA3.");
        return 1;
    }
    
    /* Setup DDR: CLK, MOSI, CS as outputs. MISO as input. */
    SPI_DDR |= (SPI_CLK | SPI_MOSI | SPI_CS);
    SPI_DDR &= ~SPI_MISO;
    
    SPI_PORT |= SPI_CS;  /* CS high */
    SPI_PORT &= ~SPI_CLK; /* CLK low */
    
    tx_val = (uint8_t)parse_hex(args[1]);
    rx_val = spi_transfer(tx_val);
    
    print_str("TX: 0x");
    print_hex_byte(tx_val);
    print_str(" -> RX: 0x");
    print_hex_byte(rx_val);
    println("");
    
    return 0;
}
