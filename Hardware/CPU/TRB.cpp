#include "TRB.h"

void TRB::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    Byte res = cpu.A & dato;

    cpu.Z = (res == 0);

    Byte res2 = (~cpu.A) & dato;
    mem.Write(ZP_Dir, res2);
}

void TRB::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Byte dato = cpu.LeerByte(Dir, mem);
    Byte res = cpu.A & dato;

    cpu.Z = (res == 0);

    Byte res2 = (~cpu.A) & dato;
    mem.Write(Dir, res2);
}
