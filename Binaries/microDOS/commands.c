#include "commands.h"
#include "../Libs/BIOS.h"
#include <string.h>

/* Command table */
static const command_t command_table[] = {
    {"ls",    cmd_ls},
    {"cat",   cmd_cat},
    {"touch", cmd_touch},
    {"mkdir", cmd_mkdir},
    {"cd",    cmd_cd},
    {"rm",    cmd_rm},
    {"nano",  cmd_nano},
    {"ping",  cmd_ping},
    {"wifi",  cmd_wifi},
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

    print_str("Unknown command: ");
    println(name);
}

void cmd_help(void) {
    println("Commands: ls, cd, mkdir, touch, cat, rm, nano, ping, wifi, help, exit");
}

void cmd_exit(void) {
    /* Main loop in microDOS.c handles exit if we break it, 
       but we can also signal it here if needed. 
       For now, microDOS.c checks for "exit" explicitly or we could use a global. 
       Let's keep it simple and just print a goodbye. */
    println("Exiting microDOS...");
}