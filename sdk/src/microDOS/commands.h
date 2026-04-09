#ifndef COMMANDS_H
#define COMMANDS_H

typedef void (*cmd_func_t)(void);

typedef struct {
    const char* name;
    cmd_func_t func;
} command_t;

/* Registry */
void exec_command(const char* name);
int  try_run_app(const char* name);

/* Built-in commands (kept in ROM) */
void cmd_ls(void);
void cmd_cat(void);
void cmd_touch(void);
void cmd_mkdir(void);
void cmd_cd(void);
void cmd_rm(void);
void cmd_wifi(void);
void cmd_run(void);
void cmd_help(void);
void cmd_exit(void);

#endif /* COMMANDS_H */