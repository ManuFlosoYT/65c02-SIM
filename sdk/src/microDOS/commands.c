#include "commands.h"
#include "../Libs/BIOS.h"
#include "../Libs/SD.h"
#include <string.h>

static const command_t command_table[] = {
    {"run",   cmd_run},
    {"help",  cmd_help},
    {"exit",  cmd_exit},
    {NULL,    NULL}
};

void exec_command(const char* name) {
    int i = 0;
    while (command_table[i].name != NULL) {
        if (strcmp(command_table[i].name, name) == 0) {
            command_table[i].func();
            return;
        }
        i++;
    }

    /* Transparent fallback: try /bin/<name>.app on the SD card */
    if (!try_run_app(name)) {
        print_str("Error: Unknown command or application '");
        print_str(name);
        println("'");
    }
}

void cmd_help(void) {
    SD_DIR dir;
    SD_INFO fno;
    print_str(os_get_msg(24));
    if (sd_opendir(&dir, "/bin")) {
        while (sd_readdir(&dir, &fno)) {
            if (!(fno.fattrib & AM_DIR)) {
                print_str("  "); println(fno.fname);
            }
        }
        sd_closedir(&dir);
    } else {
        println("  (Directory /bin not found)");
    }
}

void cmd_exit(void) {
    println("Exiting microDOS... Goodbye!");
}