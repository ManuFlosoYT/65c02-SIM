#include "CPU.h"

#include <iostream>

#include "../Instrucciones/JSR.h"
#include "../Instrucciones/LDA.h"
#include "../Instrucciones/NOP.h"
#include "../Instrucciones/LDX.h"
#include "../Instrucciones/LDY.h"
#include "../Instrucciones/JMP.h"
#include "../Instrucciones/STA.h"

void CPU::Ejecutar(Mem& mem) {
    while (true) {
        Byte opcode = FetchByte(mem);

        switch (opcode) {
            case INS_NOP: {
                NOP::Ejecutar(*this, mem);
                break;
            }
            case INS_LDA_IM: {
                LDA::EjecutarInmediato(*this, mem);
                break;
            }
            case INS_LDA_ZP: {
                LDA::EjecutarZP(*this, mem);
                break;
            }
            case INS_LDA_ZPX: {
                LDA::EjecutarZPX(*this, mem);
                break;
            }
            case INS_LDA_ABS: {
                LDA::EjecutarABS(*this, mem);
                break;
            }
            case INS_LDA_ABSX: {
                LDA::EjecutarABSX(*this, mem);
                break;
            }
            case INS_LDA_ABSY: {
                LDA::EjecutarABSY(*this, mem);
                break;
            }
            case INS_LDA_INDX: {
                LDA::EjecutarINDX(*this, mem);
                break;
            }
            case INS_LDA_INDY: {
                LDA::EjecutarINDY(*this, mem);
                break;
            }
            case INS_JSR: {
                JSR::Ejecutar(*this, mem);
                break;
            }
            case INS_LDX_IM: {
                LDX::EjecutarInmediato(*this, mem);
                break;
            }
            case INS_LDX_ZP: {
                LDX::EjecutarZP(*this, mem);
                break;
            }
            case INS_LDX_ZPY: {
                LDX::EjecutarZPY(*this, mem);
                break;
            }
            case INS_LDX_ABS: {
                LDX::EjecutarABS(*this, mem);
                break;
            }
            case INS_LDX_ABSY: {
                LDX::EjecutarABSY(*this, mem);
                break;
            }
            case INS_LDY_IM: {
                LDY::EjecutarInmediato(*this, mem);
                break;
            }
            case INS_LDY_ZP: {
                LDY::EjecutarZP(*this, mem);
                break;
            }
            case INS_LDY_ZPX: {
                LDY::EjecutarZPX(*this, mem);
                break;
            }
            case INS_LDY_ABS: {
                LDY::EjecutarABS(*this, mem);
                break;
            }
            case INS_LDY_ABSX: {
                LDY::EjecutarABSX(*this, mem);
                break;
            }
            case INS_JMP_ABS: {
                JMP::EjecutarABS(*this, mem);
                break;
            }
            case INS_JMP_IND: {
                JMP::EjecutarIND(*this, mem);
                break;
            }
            case INS_JMP_ABSX: {
                JMP::EjecutarABSX(*this, mem);
                break;
            }
            case INS_STA_ZP: {
                STA::EjecutarZP(*this, mem);
                break;
            }
            case INS_STA_ZPX: {
                STA::EjecutarZPX(*this, mem);
                break;
            }
            case INS_STA_ABS: {
                STA::EjecutarABS(*this, mem);
                break;
            }
            case INS_STA_ABSX: {
                STA::EjecutarABSX(*this, mem);
                break;
            }
            case INS_STA_ABSY: {
                STA::EjecutarABSY(*this, mem);
                break;
            }
            case INS_STA_INDX: {
                STA::EjecutarINDX(*this, mem);
                break;
            }
            case INS_STA_INDY: {
                STA::EjecutarINDY(*this, mem);
                break;
            }
            case INS_STA_IND_ZP: {
                STA::EjecutarINDZP(*this, mem);
                break;
            }
            default:

#ifndef TESTING_ENV
                std::cout << "Opcode desconocido: 0x" << std::hex
                          << static_cast<int>(opcode) << " PC: 0x" << PC
                          << std::dec << " ejecución cancelada." << std::endl;
#endif

                return;
        }
    }
}

void CPU::PushByte(Byte val, Mem& mem) {
    mem[SP] = val;
    SP--;
}

Byte CPU::PopByte(Mem& mem) {
    SP++;
    return mem[SP];
}

void CPU::PushWord(Word val, Mem& mem) {
    PushByte((val >> 8) & 0xFF, mem);
    PushByte(val & 0xFF, mem);
}

Word CPU::PopWord(Mem& mem) {
    Word Low = PopByte(mem);
    Word High = PopByte(mem);
    return (High << 8) | Low;
}


void CPU::Reset(Mem& mem) {
    PC = 0xFFFC;  // Dirección de reinicio
    SP = 0x01FF;  // Inicio de pila (Top of Stack)

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
}

const Byte CPU::FetchByte(const Mem& mem) {
    Byte dato = mem[PC];
    PC++;
    return dato;
}

const Byte CPU::LeerByte(const Word dir, const Mem& mem) {
    Byte dato = mem[dir];
    return dato;
}

const Word CPU::FetchWord(const Mem& mem) {
    Byte Dato_Low = mem[PC];
    PC++;
    Byte Dato_High = mem[PC];
    PC++;
    Word dato = (Dato_High << 8) | Dato_Low;
    return dato;
}

const Word CPU::LeerWord(const Word dir, const Mem& mem) {
    Byte Dato_Low = mem[dir];
    Byte Dato_High = mem[dir + 1];
    Word dato = (Dato_High << 8) | Dato_Low;
    return dato;
}