#ifndef COMMANDS_H
#define COMMANDS_H

#include "shell.h"

typedef void (*cmd_func_t)(void);

typedef struct {
    const char* name;
    cmd_func_t func;
} command_t;

/* Registry functions */
void exec_command(const char* name);

/* Command prototypes */
void cmd_ls(void);
void cmd_cat(void);
void cmd_touch(void);
void cmd_mkdir(void);
void cmd_cd(void);
void cmd_rm(void);
void cmd_nano(void);
void cmd_ping(void);
void cmd_wifi(void);
void cmd_help(void);
void cmd_exit(void);

#endif /* COMMANDS_H */