#include <stdint.h>
#include "../Libs/app-bios.h"

MICRODOS_NO_DESKTOP();

int main(void) {
    char buf[64];
    if (sd_getcwd(buf, sizeof(buf)) && buf[0] != '\0') {
        println(buf);
    } else {
        println("/");
    }
    return 0;
}
