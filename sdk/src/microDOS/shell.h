#ifndef SHELL_H
#define SHELL_H

#define MAX_LINE 64
#define MAX_ARGS 4

/* Variables globales exportadas */
extern char line[MAX_LINE];
extern char* args[MAX_ARGS];
extern int arg_count;

/* Prototipos */
void tokenize(char* buf);
void auto_complete(char* buf, int* len, int max_len);
void os_read_line(char* buf, int max_len);

#endif /* SHELL_H */