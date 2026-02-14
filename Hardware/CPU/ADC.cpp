#include "ADC.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Word res, Byte dato, Byte oldA) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
    cpu.C = (res > 0xFF);
    cpu.V = (~(oldA ^ dato) & (oldA ^ res) & 0b10000000) > 0;
}

static void SetFlagsBCD(CPU& cpu, Word binaryRes, Byte dato, Byte oldA) {
    cpu.Z = ((binaryRes & 0xFF) == 0);
    cpu.N = (binaryRes & 0b10000000) > 0;
    cpu.V = (~(oldA ^ dato) & (oldA ^ binaryRes) & 0b10000000) > 0;
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

void ADC::ExecuteImmediate(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);
    Byte oldA = cpu.A;

    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

void ADC::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

void ADC::ExecuteZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

void ADC::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte(Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

void ADC::ExecuteABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte(Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

void ADC::ExecuteABSY(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.Y;
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte(Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

void ADC::ExecuteINDX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte oldA = cpu.A;

    Word Dir = cpu.ReadWord(ZP_Dir, mem);

    Byte dato = cpu.ReadByte(Dir, mem);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

void ADC::ExecuteINDY(CPU& cpu, Mem& mem) {
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
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

void ADC::ExecuteIND_ZP(CPU& cpu, Mem& mem) {
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
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

}  // namespace Hardware::Instructions
