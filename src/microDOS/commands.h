#ifndef COMMANDS_H
#define COMMANDS_H

typedef void (*cmd_func_t)(void);

typedef struct {
    const char* name;
    cmd_func_t func;
} command_t;

#include <stdbool.h>

/* Registry */
void exec_command(const char* name);
bool try_run_app(const char* name);

/* Built-in commands (kept in ROM) */
void cmd_run(void);
void cmd_help(void);
void cmd_exit(void);

#endif /* COMMANDS_H */