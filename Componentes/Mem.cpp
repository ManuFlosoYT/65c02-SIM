//
// Created by manu on 29/1/26.
//

#include "Mem.h"

#include <iostream>

void Mem::Init() {
    std::cout << "Inicializando memoria a cero." << std::endl;
    for (Byte& i : memoria) {
        i = 0;
    }
}
