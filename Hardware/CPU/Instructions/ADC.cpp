#include "ADC.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

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

void ADC::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte(bus);
    Byte oldA = cpu.A;

    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

void ADC::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte(ZP_Dir, bus);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

void ADC::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    ZP_Dir += cpu.X;
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte(ZP_Dir, bus);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

void ADC::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte(Dir, bus);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

void ADC::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte(Dir, bus);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

void ADC::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte(effectiveAddr, bus);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

void ADC::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    ZP_Dir += cpu.X;
    Byte oldA = cpu.A;

    Word Dir = cpu.ReadWord(ZP_Dir, bus);

    Byte dato = cpu.ReadByte(Dir, bus);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

void ADC::ExecuteINDY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte oldA = cpu.A;

    Word baseAddr;

    if (ZP_Dir != 0xFF) {
        baseAddr = cpu.ReadWord(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte(0xFF, bus);
        Byte high = cpu.ReadByte(0x00, bus);
        baseAddr = (high << 8) | low;
    }

    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Byte dato = cpu.ReadByte(effectiveAddr, bus);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

void ADC::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte oldA = cpu.A;

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte(0xFF, bus);
        Byte high = cpu.ReadByte(0x00, bus);
        dir = (high << 8) | low;
    }

    Byte dato = cpu.ReadByte(dir, bus);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A + dato + cpu.C;
        res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

}  // namespace Hardware::Instructions
