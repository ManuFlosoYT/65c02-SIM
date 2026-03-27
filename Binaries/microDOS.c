#include <string.h>
#include "Libs/BIOS.h"
#include "Libs/SD.h"
#include "microDOS/shell.h"
#include "microDOS/commands.h"

int main(void) {
    char cwd[64];
    int res = -1;
    
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

        if (arg_count == 0) {
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            cmd_exit();
            break;
        }

        exec_command(args[0]);
    }

    return 0;
}