#include "SBC.h"

static void SetFlags(CPU& cpu, Word res, Byte dato, Byte oldA) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
    cpu.C = !(res > 0xFF);
    cpu.V = ((oldA ^ res) & (oldA ^ dato) & 0b10000000) > 0;
}

static void SetFlagsBCD(CPU& cpu, Word res, Byte dato, Byte oldA) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
    cpu.V = ((oldA ^ res) & (oldA ^ dato) & 0b10000000) > 0;
}

static Word SBC_Decimal(CPU& cpu, Byte dato) {
    bool carry = cpu.C;

    // Binary subtraction for Carry flag
    Word bin_diff = cpu.A - dato - (1 - (carry ? 1 : 0));

    // Carry is cleared if overflow (borrow) occurred
    bool no_borrow = !(bin_diff > 0xFF);
    cpu.C = no_borrow;

    int l = (cpu.A & 0x0F) - (dato & 0x0F) - (1 - (carry ? 1 : 0));
    int h = (cpu.A >> 4) - (dato >> 4);

    if (l < 0) {
        l -= 6;
        h--;
    }

    if (h < 0) {
        h -= 6;
    }

    Word res = ((h & 0x0F) << 4) | (l & 0x0F);
    return res;
}

void SBC::EjecutarInmediato(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);
    Byte oldA = cpu.A;

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

void SBC::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte oldA = cpu.A;

    Byte dato = cpu.LeerByte(ZP_Dir, mem);
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

void SBC::EjecutarZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte oldA = cpu.A;

    Byte dato = cpu.LeerByte(ZP_Dir, mem);
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

void SBC::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Byte oldA = cpu.A;

    Byte dato = cpu.LeerByte(Dir, mem);
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

void SBC::EjecutarABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;
    Byte oldA = cpu.A;

    Byte dato = cpu.LeerByte(Dir, mem);
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

void SBC::EjecutarABSY(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.Y;
    Byte oldA = cpu.A;

    Byte dato = cpu.LeerByte(Dir, mem);
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

void SBC::EjecutarINDX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte oldA = cpu.A;

    Word Dir = cpu.LeerWord(ZP_Dir, mem);

    Byte dato = cpu.LeerByte(Dir, mem);
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

void SBC::EjecutarINDY(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte oldA = cpu.A;

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.LeerWord(ZP_Dir, mem);
    } else {
        Byte low = cpu.LeerByte(0xFF, mem);
        Byte high = cpu.LeerByte(0x00, mem);
        dir = (high << 8) | low;
    }

    dir += cpu.Y;

    Byte dato = cpu.LeerByte(dir, mem);
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

void SBC::EjecutarIND_ZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte oldA = cpu.A;

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.LeerWord(ZP_Dir, mem);
    } else {
        Byte low = cpu.LeerByte(0xFF, mem);
        Byte high = cpu.LeerByte(0x00, mem);
        dir = (high << 8) | low;
    }

    Byte dato = cpu.LeerByte(dir, mem);
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