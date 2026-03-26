#include <string.h>
#include "Libs/BIOS.h"
#include "Libs/SD.h"
#include "shell.h"
#include "commands.h"

int main(void) {
    char cwd[64];
    int res;
    
    /* Asumo que esto es un macro definido en BIOS.h o similar */
    INIT_BUFFER();

    println("microDOS v1.1");
    println("Mounting SD...");
    
    if (!sd_is_present()) {
        println("ERROR: SD Hardware not detected at 0x5100!");
        return 1;
    }

    res = sd_mount();
    if (res != 0) {
        print_str("CRITICAL: Could not mount SD card: ");
        println(sd_error_string(res));
        return 1;
    }

    println("Ready.");

    while (1) {
        if (!sd_getcwd(cwd, 64)) {
            strcpy(cwd, "?");
        }
        print_str("microDOS: ");
        print_str(cwd);
        print_str(" > ");

        os_read_line(line, MAX_LINE);
        tokenize(line);

        if (arg_count == 0) continue;

        if (strcmp(args[0], "ls") == 0) {
            cmd_ls();
        } else if (strcmp(args[0], "cat") == 0) {
            cmd_cat();
        } else if (strcmp(args[0], "touch") == 0) {
            cmd_touch();
        } else if (strcmp(args[0], "mkdir") == 0) {
            cmd_mkdir();
        } else if (strcmp(args[0], "cd") == 0) {
            cmd_cd();
        } else if (strcmp(args[0], "rm") == 0) {
            cmd_rm();
        } else if (strcmp(args[0], "nano") == 0) {
            cmd_nano();
        } else if (strcmp(args[0], "help") == 0) {
            println("Commands: ls, cd, mkdir, touch, cat, rm, nano, help, exit");
        } else if (strcmp(args[0], "exit") == 0) {
            break;
        } else {
            print_str("Unknown command: ");
            println(args[0]);
        }
    }

    return 0;
}