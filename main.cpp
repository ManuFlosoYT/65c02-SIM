//
// Created by manu on 29/1/26.
//

#include <iostream>
#include <vector>
#include <memory>

#include "Instrucciones/Instruccion.h"
#include "Instrucciones/NOP.h"

int main() {
    std::vector<std::unique_ptr<Instruccion>> lista;

    lista.push_back(std::make_unique<NOP>());
    lista.push_back(std::make_unique<NOP>());

    std::cout << "--- Ejecutando instrucciones ---" << std::endl;

    for (const auto& entidad : lista) {
        entidad -> Ejecutar();
    }
}