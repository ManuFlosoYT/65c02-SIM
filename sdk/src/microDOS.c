#include <string.h>
#include "Libs/BIOS.h"
#include "Libs/SD.h"
#include "microDOS/shell.h"
#include "microDOS/commands.h"
#include "microDOS/msg.h"

int main(void) {
    char cwd[64];
    int res = -1;
    
    INIT_BUFFER();

    println("microDOS");
    print_str("Mounting SD Card...");
    
    if (!sd_is_present()) {
        print_str(M_ERR); println("SD Card not found");
        return 1;
    }

    res = sd_mount();
    if (res != 0) {
        print_str(M_ERR); print_str("Mounting failed: ");
        println(sd_error_string(res));
        return 1;
    }

    println(M_RDY);

    while (1) {
        if (!sd_getcwd(cwd, 64)) {
            strcpy(cwd, "?");
        }
        print_str(M_PROMPT);
        print_str(cwd);
        print_str(" > ");

        os_read_line(line, MAX_LINE);
        tokenize(line);

        if (arg_count == 0) {
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            cmd_exit();
            break;
        }

        exec_command(args[0]);
    }

    return 0;
}