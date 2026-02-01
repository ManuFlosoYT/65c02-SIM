#ifndef MIPS_SIM_65C02_CPU_H
#define MIPS_SIM_65C02_CPU_H
#include <cstdint>

#include "../Instrucciones/ListaInstrucciones.h"
#include "Mem.h"

using Byte = uint8_t;
using Word = uint16_t;

class CPU {
public:
    Word PC{};  // Program Counter
    Word SP{};  // Stack Pointer

    Byte A{}, X{}, Y{};  // Registros

    Byte C : 1;  // Carry Flag
    Byte Z : 1;  // Zero Flag
    Byte I : 1;  // Interrupt Disable
    Byte D : 1;  // Decimal Mode Flag
    Byte B : 1;  // Break Command
    Byte V : 1;  // Overflow Flag
    Byte N : 1;  // Negative Flag    

    CPU() = default;
    void Reset(Mem& mem);
    void Ejecutar(Mem& mem);

    const Byte GetStatus() const;
    void SetStatus(Byte status);

    const Byte FetchByte(
        const Mem& mem);  // Lee un byte de memoria y avanza el PC
    const Word FetchWord(
        const Mem& mem);  // Lee un word de memoria y avanza el PC + 2

    const Byte LeerByte(
        const Word dir,
        const Mem& mem);  // Lee un byte de memoria sin avanzar el PC
    const Word LeerWord(
        const Word dir,
        const Mem& mem);  // Lee un word de memoria sin avanzar el PC

    // Stack Operations
    void PushByte(Byte val, Mem& mem);
    Byte PopByte(Mem& mem);
    void PushWord(Word val, Mem& mem);
    Word PopWord(Mem& mem);
};

#endif  // MIPS_SIM_65C02_CPU_H