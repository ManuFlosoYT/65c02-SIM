#include "SBC.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Word res, Byte dato, Byte oldA) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
    cpu.C = !(res > 0xFF);
    cpu.V = ((oldA ^ res) & (oldA ^ dato) & 0b10000000) > 0;
}

static void SetFlagsBCD(CPU& cpu, Word binaryRes, Byte dato, Byte oldA) {
    cpu.Z = ((binaryRes & 0xFF) == 0);
    cpu.N = (binaryRes & 0b10000000) > 0;
    cpu.V = ((oldA ^ binaryRes) & (oldA ^ dato) & 0b10000000) > 0;
}

static Word SBC_Decimal(CPU& cpu, Byte dato) {
    bool acarreoPrevio = cpu.C;

    Word diferenciaBinariaCompleta =
        cpu.A - dato - (1 - (acarreoPrevio ? 1 : 0));

    bool noHuboPrestamo = !(diferenciaBinariaCompleta > 0xFF);
    cpu.C = noHuboPrestamo;

    int diferenciaNibbleBajo =
        (cpu.A & 0x0F) - (dato & 0x0F) - (1 - (acarreoPrevio ? 1 : 0));
    int diferenciaNibbleAlto = (cpu.A >> 4) - (dato >> 4);

    if (diferenciaNibbleBajo < 0) {
        diferenciaNibbleBajo -= 6;
        diferenciaNibbleAlto--;
    }

    if (diferenciaNibbleAlto < 0) {
        diferenciaNibbleAlto -= 6;
    }

    Word resultadoFinal =
        ((diferenciaNibbleAlto & 0x0F) << 4) | (diferenciaNibbleBajo & 0x0F);
    return resultadoFinal;
}

void SBC::ExecuteImmediate(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);
    Byte oldA = cpu.A;

    Word res;
    if (cpu.D == 0) {
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

void SBC::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

void SBC::ExecuteZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

void SBC::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte(Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

void SBC::ExecuteABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte(Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

void SBC::ExecuteABSY(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.Y;
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte(Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

void SBC::ExecuteINDX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte oldA = cpu.A;

    Word Dir = cpu.ReadWord(ZP_Dir, mem);

    Byte dato = cpu.ReadByte(Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

void SBC::ExecuteINDY(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte oldA = cpu.A;

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord(ZP_Dir, mem);
    } else {
        Byte low = cpu.ReadByte(0xFF, mem);
        Byte high = cpu.ReadByte(0x00, mem);
        dir = (high << 8) | low;
    }

    dir += cpu.Y;

    Byte dato = cpu.ReadByte(dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

void SBC::ExecuteIND_ZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte oldA = cpu.A;

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord(ZP_Dir, mem);
    } else {
        Byte low = cpu.ReadByte(0xFF, mem);
        Byte high = cpu.ReadByte(0x00, mem);
        dir = (high << 8) | low;
    }

    Byte dato = cpu.ReadByte(dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, res, dato, oldA);
    }
}

}  // namespace Hardware::Instructions
