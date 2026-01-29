//
// Created by manu on 29/1/26.
//

#include "CPU.h"

#include <iostream>

void CPU::Reset( Mem& mem ) {
    std::cout << "Reseteando CPU..." << std::endl;
    PC = 0xFFFC; // Dirección de reinicio
    SP = 0x0100; // Inicio de pila

    // Reset de registros
    A = 0;
    X = 0;
    Y = 0;

    // Reset de flags
    C = 0;
    Z = 0;
    I = 0;
    D = 0;
    B = 0;
    V = 0;
    N = 0;

    mem.Init();

    lista.push_back(std::make_unique<NOP>());
    lista.push_back(std::make_unique<NOP>());
}

void CPU::Ejecutar() const {
    for (const auto& entidad : lista) {
        entidad -> Ejecutar();
    }
}
