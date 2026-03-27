#include "commands.h"

#include "../Libs/BIOS.h"
#include "../Libs/NET.h"
#include "../Libs/SD.h"
#include "shell.h"

void cmd_ping(void) {
    uint8_t i;
    uint16_t min_ping = 65535, max_ping = 0, total_ping = 0;
    uint8_t lost = 0;
    uint16_t current;
    char chr;

    if (arg_count < 2) {
        println("Usage: ping <host|ip>");
        return;
    }

    print_str("Pinging ");
    print_str(args[1]);
    println(":");

    for (i = 0; i < 10; ++i) {
        net_send("AT+PING=\"");
        net_send(args[1]);
        net_cmd("\"");

        /* Wait for response start '+' */
        while (1) {
            if (net_has_data()) {
                chr = (char)net_getc();
                if (chr == '+') {
                    /* Check next character to skip echo (AT+PING) vs real response (+32) */
                    while (!net_has_data());
                    chr = (char)net_getc();
                    if (chr >= '0' && chr <= '9') {
                        /* Real Response! Parse digits */
                        current = (uint16_t)(chr - '0');
                        while (1) {
                            if (net_has_data()) {
                                chr = (char)net_getc();
                                if (chr >= '0' && chr <= '9') {
                                    current = current * 10 + (uint16_t)(chr - '0');
                                } else
                                    break; /* End of digits */
                            }
                        }
                        print_str("Reply from ");
                        print_str(args[1]);
                        print_str(": time=");
                        print_num(current);
                        println("ms");

                        if (current < min_ping) min_ping = current;
                        if (current > max_ping) max_ping = current;
                        total_ping += current;
                        goto next_ping;
                    } else if (chr == 't') {
                        /* timeout starts with 't' */
                        print_str("Reply from ");
                        print_str(args[1]);
                        println(": Request timed out.");
                        lost++;
                        goto next_ping;
                    }
                } else if (chr == 'E') {
                    /* ERROR detected */
                    print_str("Reply from ");
                    print_str(args[1]);
                    println(": General error.");
                    lost++;
                    goto next_ping;
                }
            }
        }

    next_ping:
        /* Wait for current command to fully finish (OK or ERROR) before next one */
        while (1) {
            if (net_has_data()) {
                chr = (char)net_getc();
                if (chr == 'K' || chr == 'R') break;
            }
        }
        delay(10);
    }

    /* Print Summary Stats */
    println("\nPing statistics:");
    print_str("    Packets: Sent = 10, Received = ");
    print_num(10 - lost);
    print_str(", Lost = ");
    print_num(lost);
    print_str(" (");
    print_num(lost * 10);
    println("% loss)");

    if (lost < 10) {
        println("Approximate round trip times in milli-seconds:");
        print_str("    Min=");
        print_num(min_ping);
        print_str("ms, Max=");
        print_num(max_ping);
        print_str("ms, Avg=");
        print_num(total_ping / (10 - lost));
        println("ms");
    }
}

void cmd_wifi(void) {
    char chr;
    if (arg_count < 3) {
        println("Usage: wifi <ssid> <password>");
        return;
    }
    print_str("Connecting to ");
    print_str(args[1]);
    println("...");

    net_wifi(args[1], args[2]);

    /* Wait for OK or ERROR */
    while (1) {
        if (net_has_data()) {
            chr = (char)net_getc();
            if (chr == '\r') continue;
            bios_putchar(chr); /* Show response: WIFI CONNECTED, etc. */
            if (chr == 'K') {
                println("\nConnected.");
                break;
            }
            if (chr == 'R') {
                println("\nFailed.");
                break;
            }
        }
    }
}

void cmd_ls(void) {
    SD_DIR dir;
    SD_INFO fno;
    char* path;
    char size_str[11];
    unsigned long sz;
    int i;

    path = (arg_count > 1) ? args[1] : ".";

    if (sd_opendir(&dir, path)) {
        while (sd_readdir(&dir, &fno)) {
            if (fno.fattrib & AM_DIR) {
                print_str("[DIR]      ");
            } else {
                sz = fno.fsize;
                i = 9;
                size_str[10] = '\0';
                for (; i >= 0; i--) {
                    size_str[i] = (sz > 0 || i == 9) ? (char)((sz % 10) + '0') : ' ';
                    sz /= 10;
                }
                print_str(size_str);
                print_str(" ");
            }
            println(fno.fname);
        }
        sd_closedir(&dir);
    } else {
        println("ERROR: ls failed");
    }
}

void cmd_cat(void) {
    SD_FILE fp;
    int c;
    if (arg_count < 2) {
        println("Usage: cat <file>");
        return;
    }
    if (sd_open(&fp, args[1], SD_READ)) {
        while ((c = sd_getc(&fp)) != -1) {
            bios_putchar((char)c);
        }
        println("");
        sd_close(&fp);
    } else {
        println("ERROR: cat failed");
    }
}

void cmd_touch(void) {
    SD_FILE fp;
    if (arg_count < 2) {
        println("Usage: touch <file>");
        return;
    }
    if (sd_open(&fp, args[1], SD_WRITE | SD_CREATE_ALWAYS)) {
        sd_close(&fp);
    } else {
        println("ERROR: touch failed");
    }
}

void cmd_mkdir(void) {
    if (arg_count < 2) {
        println("Usage: mkdir <dir>");
        return;
    }
    if (!sd_mkdir(args[1])) {
        println("ERROR: mkdir failed");
    }
}

void cmd_cd(void) {
    if (arg_count < 2) {
        println("Usage: cd <dir>");
        return;
    }
    if (!sd_chdir(args[1])) {
        println("ERROR: cd failed");
    }
}

void cmd_rm(void) {
    if (arg_count < 2) {
        println("Usage: rm <path>");
        return;
    }
    if (!sd_remove(args[1])) {
        println("ERROR: rm failed");
    }
}

void cmd_nano(void) {
    SD_FILE fp;
    char c;
    static char editor_buf[4096];
    int editor_len = 0;

    if (arg_count < 2) {
        println("Usage: nano <file>");
        return;
    }

    if (!sd_open(&fp, args[1], SD_WRITE | SD_CREATE_ALWAYS)) {
        println("ERROR: nano failed (open)");
        return;
    }

    println("--- NANO EDITOR ---");
    println("Type your text. Press ESC to save and exit.");

    while (1) {
        c = bios_getchar();
        if (c == 27) break;

        if (c == '\b' || c == 8 || c == 127) {
            if (editor_len > 0) {
                editor_len--;
                bios_putchar(' ');
                bios_putchar('\b');
            }
        } else if (c == '\r' || c == '\n') {
            if (editor_len < sizeof(editor_buf) - 1) {
                editor_buf[editor_len++] = '\n';
                bios_putchar('\r');
                bios_putchar('\n');
            }
        } else {
            if (editor_len < sizeof(editor_buf) - 1) {
                editor_buf[editor_len++] = c;
            }
        }
    }

    editor_buf[editor_len] = '\0';
    sd_puts(&fp, editor_buf);

    sd_close(&fp);
    println("");
    println("--- SAVED ---");
}