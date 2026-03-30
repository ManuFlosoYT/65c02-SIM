#include "commands.h"
#include "../Libs/BIOS.h"
#include "../Libs/SD.h"
#include <string.h>

static const command_t command_table[] = {
    {"ls",    cmd_ls},
    {"cat",   cmd_cat},
    {"touch", cmd_touch},
    {"mkdir", cmd_mkdir},
    {"cd",    cmd_cd},
    {"rm",    cmd_rm},
    {"wifi",  cmd_wifi},
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

    println("microDOS Shell Commands:");
    println("  ls [dir]: List directory contents");
    println("  cd <dir>: Change working directory");
    println("  mkdir <dir>: Create new directory");
    println("  touch <file>: Create a new empty file");
    println("  cat <file>: Display file contents");
    println("  rm <path>: Remove file or directory");
    println("  wifi <SSID> <PASSWORD>: Connect to WiFi network");
    println("  run <app>: Load and execute SD application");
    println("  help: Show this help information");
    println("  exit: Reboot the system");

    println("\nApplications in /bin/:");
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