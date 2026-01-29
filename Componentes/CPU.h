//
// Created by manu on 29/1/26.
//

#ifndef MIPS_SIM_65C02_CPU_H
#define MIPS_SIM_65C02_CPU_H
#include <cstdint>
#include <vector>
#include <memory>

#include "Mem.h"

#include "../Instrucciones/ListaInstrucciones.h"
#include "../Instrucciones/Instruccion.h"
#include "../Instrucciones/NOP.h"

using Byte = uint8_t;
using Word = uint16_t;

class CPU {
    private:
        Word PC{}; // Program Counter
        Word SP{}; // Stack Pointer

        Byte A{}, X{}, Y{}; // Registros

        Byte C : 1{}; // Carry Flag
        Byte Z : 1{}; // Zero Flag
        Byte I : 1{}; // Interrupt Disable
        Byte D : 1{}; // Decimal Mode Flag
        Byte B : 1{}; // Break Command
        Byte V : 1{}; // Overflow Flag
        Byte N : 1{}; // Negative Flag

        std::vector<std::unique_ptr<Instruccion>> lista;

    public:
        CPU() = default;
        void Reset( Mem& mem );
        void Ejecutar( Mem& mem );

        Byte FetchByte( Mem& mem ); // Lee un byte de memoria y avanza el PC
};


#endif //MIPS_SIM_65C02_CPU_H