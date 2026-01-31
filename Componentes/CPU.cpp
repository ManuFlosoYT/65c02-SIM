#include <iostream>

#include "../Instrucciones/LDA.h"
#include "../Instrucciones/NOP.h"
#include "CPU.h"

void CPU::Reset(Mem &mem) {
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
}

void CPU::Ejecutar(Mem &mem) {
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
                std::cout << "Opcode desconocido: 0x"
                    << std::hex                 // Cambiar a formato hexadecimal
                    << static_cast<int>(opcode) // Mostrar el opcode
                    << std::dec                 // Volver a formato decimal
                    << " ejecución cancelada." 
                    << std::endl;
                return; // Salir de la ejecución
        }
    }
}

Byte CPU::FetchByte(Mem &mem) {

    if (PC >= mem.MAX_MEM) {
        throw std::runtime_error("Acceso a memoria fuera de rango");
    }

    const Byte dato = mem[PC];
    PC++;
    return dato;
}
