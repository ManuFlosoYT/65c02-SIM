#include "commands.h"
#include "../Libs/BIOS.h"
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
        print_str("Unknown command: '");
        print_str(name);
        println("'");
    }
}

void cmd_help(void) {
    println("microDOS Shell Commands:");
    println("  ls [dir]    List directory contents");
    println("  cd <dir>    Change directory");
    println("  mkdir <dir> Create directory");
    println("  touch <f>  Create empty file");
    println("  cat <f>    Print file contents");
    println("  rm <f/d>   Remove file or directory");
    println("  wifi ...   Network configuration");
    println("  run <app>  Execute SD application");
    println("  help       Show this help");
    println("  exit       Reboot system");
    println("\nApps in /bin/ can be run directly.");
}

void cmd_exit(void) {
    println("Exiting microDOS... Goodbye!");
}