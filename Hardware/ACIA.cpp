#include "ACIA.h"

#include "Mem.h"

void ACIA::Inicializar(Mem& mem) {
    mem.SetWriteHook(ACIA_DATA, [this](Word dir, Byte val) {
        this->DATO = val;
        if (outputCallback) {
            outputCallback((char)val);
        }
    });

    mem.SetWriteHook(ACIA_STATUS, [this](Word dir, Byte val) { 
        this->STATUS = val; 
    });

    mem.SetWriteHook(ACIA_CMD, [this](Word dir, Byte val) { 
        this->CMD = val; 
    });

    mem.SetWriteHook(ACIA_CTRL, [this](Word dir, Byte val) { 
        this->CTRL = val; 
    });

    mem.SetReadHook(ACIA_DATA, [this, &mem](Word dir) {
        mem.memoria[ACIA_STATUS] &= ~0x80;
        return mem.memoria[ACIA_DATA];
    });

    mem.SetReadHook(ACIA_STATUS, [this, &mem](Word dir) {
        return mem.memoria[ACIA_STATUS];
    });
}