#include "SBC.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

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

template <bool Debug>
void SBC::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    Byte oldA = cpu.A;

    Word res;
    if (cpu.D == 0) {
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

template <bool Debug>
void SBC::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

template <bool Debug>
void SBC::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

template <bool Debug>
void SBC::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

template <bool Debug>
void SBC::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

template <bool Debug>
void SBC::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;
    Byte oldA = cpu.A;

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

template <bool Debug>
void SBC::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte oldA = cpu.A;

    Word Dir = cpu.ReadWord<Debug>(ZP_Dir, bus);

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    Word res;
    if (cpu.D == 0) {
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

template <bool Debug>
void SBC::ExecuteINDY(CPU& cpu, Bus& bus) {
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
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

template <bool Debug>
void SBC::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
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
        res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        // BCD mode: add 1 cycle penalty
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

template void SBC::ExecuteImmediate<true>(CPU&, Bus&);
template void SBC::ExecuteImmediate<false>(CPU&, Bus&);
template void SBC::ExecuteZP<true>(CPU&, Bus&);
template void SBC::ExecuteZP<false>(CPU&, Bus&);
template void SBC::ExecuteZPX<true>(CPU&, Bus&);
template void SBC::ExecuteZPX<false>(CPU&, Bus&);
template void SBC::ExecuteABS<true>(CPU&, Bus&);
template void SBC::ExecuteABS<false>(CPU&, Bus&);
template void SBC::ExecuteABSX<true>(CPU&, Bus&);
template void SBC::ExecuteABSX<false>(CPU&, Bus&);
template void SBC::ExecuteABSY<true>(CPU&, Bus&);
template void SBC::ExecuteABSY<false>(CPU&, Bus&);
template void SBC::ExecuteINDX<true>(CPU&, Bus&);
template void SBC::ExecuteINDX<false>(CPU&, Bus&);
template void SBC::ExecuteINDY<true>(CPU&, Bus&);
template void SBC::ExecuteINDY<false>(CPU&, Bus&);
template void SBC::ExecuteIND_ZP<true>(CPU&, Bus&);
template void SBC::ExecuteIND_ZP<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
