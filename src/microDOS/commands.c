#include "commands.h"
#include "Libs/BIOS.h"
#include "Libs/SD.h"
#include "Libs/System.h"
#include <stddef.h>
#include <string.h>

static const command_t command_table[] = {
    {"run",   cmd_run},
    {"help",  cmd_help},
    {nullptr, nullptr}
};

void exec_command(const char* name) {
    for (size_t i = 0; command_table[i].name != nullptr; ++i) {
        if (strcmp(command_table[i].name, name) == 0) {
            command_table[i].func();
            return;
        }
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