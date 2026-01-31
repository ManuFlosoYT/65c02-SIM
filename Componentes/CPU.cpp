#include <iostream>

#include "CPU.h"
#include "../Instrucciones/NOP.h"


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
}

void CPU::Ejecutar( Mem& mem ){
    while (true) {
        Byte opcode = FetchByte( mem );

        switch (opcode) {
            case INS_NOP: {
                NOP nop;
                nop.Ejecutar( *this, mem );
                break;
            }
            case INS_LDA_IM: {
                // Implementación de LDA Inmediato
                break;
            }
            default:
                std::cout
                    << "Opcode desconocido: "
                    << std::hex                     // Cambiar a formato hexadecimal
                    << static_cast<int>(opcode)     // Mostrar el opcode
                    << std::dec                     // Volver a formato decimal
                    << " ejecución cancelada."
                    << std::endl;
                return; // Salir de la ejecución
        }
    }
}

Byte CPU::FetchByte( Mem& mem ) {
    const Byte dato = mem[PC];
    PC++;
    return dato;
}
