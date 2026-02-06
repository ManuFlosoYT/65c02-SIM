#include <iostream>
#include <string>

#include "Componentes/Emulator.h"

int main(int argc, char* argv[]) {
    std::string bin;
    if (argc > 1) {
        bin = argv[1];
    } else {
        std::cout << "Introduce el nombre del binario sin extension:";
        std::cin >> bin;
    }

    Emulator emulator;
    emulator.Init(bin);
    emulator.Run();

    return 0;
}