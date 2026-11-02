#include <stdbool.h>
#include <string.h>
#include "Libs/BIOS.h"
#include "Libs/SD.h"
#include "Libs/System.h"
#include "microDOS/shell.h"
#include "microDOS/commands.h"
#include "microDOS/msg.h"

#define DESKTOP_SHELL ((bool(*)(char*))0x0200)

static bool sys_desktop_load(void) {
    SD_FILE f;
    if (!sd_open(&f, "/SYSTEM/DESKTOP/DESKTOP.SYS", SD_READ)) {
        return false;
    }
    const bool ok = (sd_read(&f, (void*)0x0200, 0x4000) > 0);
    sd_close(&f);
    return ok;
}

int main(void) {
    INIT_BUFFER();

    println("microDOS");

    if (sd_mount() != 0) {
        print_str(M_ERR); println("Disk error");
        return 1;
    }

    os_clear_swap_blocks();

    while (true) {
        if (!sys_desktop_load() || !DESKTOP_SHELL(line)) {
            print_str(M_PROMPT);
            os_read_line(line, MAX_LINE);
        }
        tokenize(line);
        if (arg_count > 0) {
            exec_command(args[0]);
        }
    }

    return 0;
}
