#include "CPU.h"

#include <iostream>

#include "../Instrucciones/LDA.h"
#include "../Instrucciones/NOP.h"

void CPU::Reset(Mem& mem) {
    PC = 0xFFFC;  // Dirección de reinicio
    SP = 0x0100;  // Inicio de pila

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
    if (PC >= mem.MAX_MEM) {
        PC = 0x0000;
    }

    const Byte dato = mem[PC];
    PC++;
    return dato;
}

const Byte CPU::LeerByte(const Word dir, const Mem& mem) {
    const Byte dato = mem[dir];
    return dato;
}

void CPU::Ejecutar(Mem& mem) {
    while (true) {
        Byte opcode = FetchByte(mem);

        switch (opcode) {
            case INS_NOP: {
                NOP nop;
                nop.Ejecutar(*this, mem);
                break;
            }
            case INS_LDA_IM: {
                LDA lda;
                lda.EjecutarInmediato(*this, mem);
                break;
            }
            case INS_LDA_ZP: {
                LDA lda;
                lda.EjecutarZP(*this, mem);
                break;
            }
            case INS_LDA_ZPX: {
                LDA lda;
                lda.EjecutarZPX(*this, mem);
                break;
            }
            case INS_LDA_ABS: {
                LDA lda;
                lda.EjecutarABS(*this, mem);
                break;
            }
            case INS_LDA_ABSX: {
                LDA lda;
                lda.EjecutarABSX(*this, mem);
                break;
            }
            case INS_LDA_ABSY: {
                LDA lda;
                lda.EjecutarABSY(*this, mem);
                break;
            }
            case INS_LDA_INDX: {
                LDA lda;
                lda.EjecutarINDX(*this, mem);
                break;
            }
            case INS_LDA_INDY: {
                LDA lda;
                lda.EjecutarINDY(*this, mem);
                break;
            }
            default:

                #ifndef TESTING_ENV
                    std::cout << "Opcode desconocido: 0x" 
                        << std::hex
                        << static_cast<int>(opcode) << " PC: 0x" << PC
                        << std::dec << " ejecución cancelada." << std::endl;
                #endif

                return;
        }
    }
}
