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

template <bool Debug>
void ADC::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
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

template <bool Debug>
void ADC::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
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

template <bool Debug>
void ADC::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
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

template <bool Debug>
void ADC::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
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

template <bool Debug>
void ADC::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
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

template <bool Debug>
void ADC::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte<Debug>(effectiveAddr, bus);
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

template <bool Debug>
void ADC::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte oldA = cpu.A;

    Word Dir = cpu.ReadWord<Debug>(ZP_Dir, bus);

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
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

template <bool Debug>
void ADC::ExecuteINDY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte oldA = cpu.A;

    Word baseAddr;

    if (ZP_Dir != 0xFF) {
        baseAddr = cpu.ReadWord<Debug>(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte<Debug>(0xFF, bus);
        Byte high = cpu.ReadByte<Debug>(0x00, bus);
        baseAddr = (high << 8) | low;
    }

    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Byte dato = cpu.ReadByte<Debug>(effectiveAddr, bus);
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

template <bool Debug>
void ADC::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte oldA = cpu.A;

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord<Debug>(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte<Debug>(0xFF, bus);
        Byte high = cpu.ReadByte<Debug>(0x00, bus);
        dir = (high << 8) | low;
    }

    Byte dato = cpu.ReadByte<Debug>(dir, bus);
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

template void ADC::ExecuteImmediate<true>(CPU&, Bus&);
template void ADC::ExecuteImmediate<false>(CPU&, Bus&);
template void ADC::ExecuteZP<true>(CPU&, Bus&);
template void ADC::ExecuteZP<false>(CPU&, Bus&);
template void ADC::ExecuteZPX<true>(CPU&, Bus&);
template void ADC::ExecuteZPX<false>(CPU&, Bus&);
template void ADC::ExecuteABS<true>(CPU&, Bus&);
template void ADC::ExecuteABS<false>(CPU&, Bus&);
template void ADC::ExecuteABSX<true>(CPU&, Bus&);
template void ADC::ExecuteABSX<false>(CPU&, Bus&);
template void ADC::ExecuteABSY<true>(CPU&, Bus&);
template void ADC::ExecuteABSY<false>(CPU&, Bus&);
template void ADC::ExecuteINDX<true>(CPU&, Bus&);
template void ADC::ExecuteINDX<false>(CPU&, Bus&);
template void ADC::ExecuteINDY<true>(CPU&, Bus&);
template void ADC::ExecuteINDY<false>(CPU&, Bus&);
template void ADC::ExecuteIND_ZP<true>(CPU&, Bus&);
template void ADC::ExecuteIND_ZP<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
