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