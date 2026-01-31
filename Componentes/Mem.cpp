#include "Mem.h"

void Mem::Init() {
    for (Byte& i : memoria) {
        i = 0;
    }
}
