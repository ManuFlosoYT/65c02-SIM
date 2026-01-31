#include <iostream>

#include "Componentes/CPU.h"
#include "Componentes/Mem.h"

int main() {
    Mem mem{};

    CPU cpu{};
    cpu.Reset( mem );

    std::cout << "--- Ejecutando instrucciones ---" << std::endl;

    cpu.Ejecutar( mem );
}