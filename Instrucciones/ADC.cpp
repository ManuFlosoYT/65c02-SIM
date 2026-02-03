#include "ADC.h"

static void SetFlags(CPU& cpu, Word res, Byte dato, Byte oldA) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
    cpu.C = (res > 0xFF);
    cpu.V = (~(oldA ^ dato) & (oldA ^ res) & 0b10000000) > 0;
}

static void SetFlagsBCD(CPU& cpu, Word res, Byte dato, Byte oldA) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
    cpu.C = (res > 0x99);
    cpu.V = (~(oldA ^ dato) & (oldA ^ res) & 0b10000000) > 0;
}

static Word ADC_Decimal(CPU& cpu, Byte dato) {
    Byte sumaNibbleBajo = (cpu.A & 0x0F) + (dato & 0x0F) + cpu.C;
    Byte sumaNibbleAlto = (cpu.A >> 4) + (dato >> 4);

    if (sumaNibbleBajo > 9) {
        sumaNibbleBajo += 6;
        sumaNibbleAlto++;
    }

    if (sumaNibbleAlto > 9) {
        sumaNibbleAlto += 6;
    }

    return (sumaNibbleAlto << 4) | sumaNibbleBajo;
}

void ADC::EjecutarInmediato(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);
    Byte oldA = cpu.A;

    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, res, dato, oldA);
    }
}

void ADC::EjecutarZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte oldA = cpu.A;

    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, res, dato, oldA);
    }
}

void ADC::EjecutarZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte oldA = cpu.A;

    Byte dato = cpu.LeerByte(ZP_Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, res, dato, oldA);
    }
}

void ADC::EjecutarABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Byte oldA = cpu.A;

    Byte dato = cpu.LeerByte(Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, res, dato, oldA);
    }
}

void ADC::EjecutarABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;
    Byte oldA = cpu.A;

    Byte dato = cpu.LeerByte(Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, res, dato, oldA);
    }
}

void ADC::EjecutarABSY(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.Y;
    Byte oldA = cpu.A;

    Byte dato = cpu.LeerByte(Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, res, dato, oldA);
    }
}

void ADC::EjecutarINDX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte oldA = cpu.A;

    Word Dir = cpu.LeerWord(ZP_Dir, mem);

    Byte dato = cpu.LeerByte(Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, res, dato, oldA);
    }
}

void ADC::EjecutarINDY(CPU& cpu, Mem& mem) {
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
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, res, dato, oldA);
    }
}

void ADC::EjecutarIND_ZP(CPU& cpu, Mem& mem) {
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
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, res, dato, oldA);
    }
}