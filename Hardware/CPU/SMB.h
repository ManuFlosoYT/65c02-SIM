#ifndef SIM_65C02_SMB_H
#define SIM_65C02_SMB_H

#include "../CPU.h"
#include "../Mem.h"

class SMB {
public:
    static void Ejecutar0(CPU& cpu, Mem& mem);
    static void Ejecutar1(CPU& cpu, Mem& mem);
    static void Ejecutar2(CPU& cpu, Mem& mem);
    static void Ejecutar3(CPU& cpu, Mem& mem);
    static void Ejecutar4(CPU& cpu, Mem& mem);
    static void Ejecutar5(CPU& cpu, Mem& mem);
    static void Ejecutar6(CPU& cpu, Mem& mem);
    static void Ejecutar7(CPU& cpu, Mem& mem);
};

#endif  // SIM_65C02_SMB_H