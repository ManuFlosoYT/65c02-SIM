/*
 * testSD.c - SD card test for the 65C02 Simulator
 *
 * Tests the entire stack SD.h -> FatFs -> diskio.c -> SPI/VIA:
 *   1. Mount the image
 *   2. Write TEST.TXT
 *   3. Read it back
 *   4. List the root directory
 *   5. Unmount
 */

#include "Libs/BIOS.h"
#include "Libs/SD.h"

int main(void) {
    SD_FILE fp;
    SD_DIR dir;
    SD_INFO fno;
    int c;

    println("== SD Card Test ==");

    // 1. Mount
    println("Mounting SD card...");
    if (!sd_mount()) {
        println("ERROR: mount");
        return 1;
    }
    println("OK: mounted");

    // 2. Write
    println("Writing TEST.TXT...");
    if (!sd_open(&fp, "TEST.TXT", SD_WRITE | SD_CREATE_ALWAYS)) {
        println("ERROR: open for writing");
        sd_unmount();
        return 1;
    }
    sd_puts(&fp, "Hello from the 65C02 simulator!\n");
    sd_puts(&fp, "FatFs + SPI bit-bang works.\n");
    sd_close(&fp);
    println("OK: written");

    // 3. Read
    println("Reading TEST.TXT...");
    if (!sd_open(&fp, "TEST.TXT", SD_READ)) {
        println("ERROR: open for reading");
        sd_unmount();
        return 1;
    }
    print_str("Content: [");
    while ((c = sd_getc(&fp)) != -1) {
        bios_putchar((char)c);
    }
    println("]");
    sd_close(&fp);

    // 4. List root directory
    println("Root directory:");
    if (sd_opendir(&dir, "/")) {
        while (sd_readdir(&dir, &fno)) {
            print_str("  ");
            println(fno.fname);
        }
        sd_closedir(&dir);
    }

    // 5. Unmount
    println("Unmounting SD card...");
    sd_unmount();
    println("OK: unmounted.");
    return 0;
}
