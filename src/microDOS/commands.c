#include "commands.h"
#include "Libs/BIOS.h"
#include "Libs/SD.h"
#include "Libs/System.h"
#include <stddef.h>
#include <string.h>

void exec_command(const char* name) {
    if (strcmp(name, "run") == 0) {
        cmd_run();
        return;
    }
    if (strcmp(name, "help") == 0) {
        cmd_help();
        return;
    }

    /* Transparent fallback: try /bin/<name>.app on the SD card */
    if (!try_run_app(name)) {
        print_str("Unknown: ");
        println(name);
    }
}

void cmd_help(void) {
    println("Apps in /bin");
}