#pragma once

#include "Hardware/CPU/Instructions/InstructionSet.h"

namespace Hardware {
class CPU;
class Bus;
}  // namespace Hardware

namespace Hardware::Instructions {

class ADC {
public:
    template <bool Debug>
    static void ExecuteImmediate(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSY(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteINDX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteINDY(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteIND_ZP(CPU& cpu, Bus& bus);

private:
    static void SetFlags(CPU& cpu, Word res, Byte dato, Byte oldA);
    static void SetFlagsBCD(CPU& cpu, Word binaryRes, Byte dato, Byte oldA);
    static Word ADC_Decimal(CPU& cpu, Byte dato);
    static void DoADC(CPU& cpu, Byte dato);
};

class SBC {
public:
    template <bool Debug>
    static void ExecuteImmediate(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSY(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteINDX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteINDY(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteIND_ZP(CPU& cpu, Bus& bus);

private:
    static void SetFlags(CPU& cpu, Word res, Byte dato, Byte oldA);
    static void SetFlagsBCD(CPU& cpu, Word binaryRes, Byte dato, Byte oldA);
    static Word SBC_Decimal(CPU& cpu, Byte dato);
    static void DoSBC(CPU& cpu, Byte dato);
};

class CMP {
public:
    template <bool Debug>
    static void ExecuteImmediate(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSY(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteINDX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteINDY(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteIND_ZP(CPU& cpu, Bus& bus);

private:
    static void SetFlags(CPU& cpu, Byte dato);
};

class CPX {
public:
    template <bool Debug>
    static void ExecuteImmediate(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);

private:
    static void SetFlags(CPU& cpu, Byte dato);
};

class CPY {
public:
    template <bool Debug>
    static void ExecuteImmediate(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);

private:
    static void SetFlags(CPU& cpu, Byte dato);
};

class INC {
public:
    template <bool Debug>
    static void ExecuteAccumulator(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);

private:
    static void SetFlags(CPU& cpu, Byte val);
};

class DEC {
public:
    template <bool Debug>
    static void ExecuteAccumulator(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);

private:
    static void SetFlags(CPU& cpu, Byte val);
};

}  // namespace Hardware::Instructions

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

inline void ADC::SetFlags(CPU& cpu, Word res, Byte dato, Byte oldA) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
    cpu.C = (res > 0xFF);
    cpu.V = (~(oldA ^ dato) & (oldA ^ res) & 0b10000000) > 0;
}

inline void ADC::SetFlagsBCD(CPU& cpu, Word binaryRes, Byte dato, Byte oldA) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
    cpu.V = (~(oldA ^ dato) & (oldA ^ binaryRes) & 0b10000000) > 0;
}

inline Word ADC::ADC_Decimal(CPU& cpu, Byte dato) {
    Byte sumaNibbleBajo = (cpu.A & 0x0F) + (dato & 0x0F) + cpu.C;
    Byte sumaNibbleAlto = (cpu.A >> 4) + (dato >> 4);

    if (sumaNibbleBajo > 9) {
        sumaNibbleBajo += 6;
        sumaNibbleAlto++;
    }

    if (sumaNibbleAlto > 9) {
        sumaNibbleAlto += 6;
    }

    return (sumaNibbleAlto << 4) | (sumaNibbleBajo & 0x0F);
}

inline void ADC::DoADC(CPU& cpu, Byte dato) {
    Byte oldA = cpu.A;
    if (cpu.D == 0) {
        Word res = cpu.A + dato + cpu.C;
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A + dato + cpu.C;
        Word res = ADC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
        cpu.C = (res > 0x99);
    }
}

template <bool Debug>
inline void ADC::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    DoADC(cpu, dato);
}

template <bool Debug>
inline void ADC::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    DoADC(cpu, dato);
}

template <bool Debug>
inline void ADC::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    DoADC(cpu, dato);
}

template <bool Debug>
inline void ADC::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    DoADC(cpu, dato);
}

template <bool Debug>
inline void ADC::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;
    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);
    Byte dato = cpu.ReadByte<Debug>(effectiveAddr, bus);
    DoADC(cpu, dato);
}

template <bool Debug>
inline void ADC::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.Y;
    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);
    Byte dato = cpu.ReadByte<Debug>(effectiveAddr, bus);
    DoADC(cpu, dato);
}

template <bool Debug>
inline void ADC::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Word Dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    DoADC(cpu, dato);
}

template <bool Debug>
inline void ADC::ExecuteINDY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Word baseAddr = cpu.ReadWordZP<Debug>(ZP_Dir, bus);
    Word effectiveAddr = baseAddr + cpu.Y;
    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);
    Byte dato = cpu.ReadByte<Debug>(effectiveAddr, bus);
    DoADC(cpu, dato);
}

template <bool Debug>
inline void ADC::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Word dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);
    Byte dato = cpu.ReadByte<Debug>(dir, bus);
    DoADC(cpu, dato);
}

inline void SBC::SetFlags(CPU& cpu, Word res, Byte dato, Byte oldA) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
    cpu.C = !(res > 0xFF);
    cpu.V = ((oldA ^ res) & (oldA ^ dato) & 0b10000000) > 0;
}

inline void SBC::SetFlagsBCD(CPU& cpu, Word binaryRes, Byte dato, Byte oldA) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
    cpu.V = ((oldA ^ binaryRes) & (oldA ^ dato) & 0b10000000) > 0;
}

inline Word SBC::SBC_Decimal(CPU& cpu, Byte dato) {
    bool acarreoPrevio = cpu.C;

    Word diferenciaBinariaCompleta = cpu.A - dato - (1 - (acarreoPrevio ? 1 : 0));

    bool noHuboPrestamo = !(diferenciaBinariaCompleta > 0xFF);
    cpu.C = noHuboPrestamo;

    int diferenciaNibbleBajo = (cpu.A & 0x0F) - (dato & 0x0F) - (1 - (acarreoPrevio ? 1 : 0));
    int diferenciaNibbleAlto = (cpu.A >> 4) - (dato >> 4);

    if (diferenciaNibbleBajo < 0) {
        diferenciaNibbleBajo -= 6;
        diferenciaNibbleAlto--;
    }

    if (diferenciaNibbleAlto < 0) {
        diferenciaNibbleAlto -= 6;
    }

    Word resultadoFinal = ((diferenciaNibbleAlto & 0x0F) << 4) | (diferenciaNibbleBajo & 0x0F);
    return resultadoFinal;
}

inline void SBC::DoSBC(CPU& cpu, Byte dato) {
    Byte oldA = cpu.A;
    if (cpu.D == 0) {
        Word res = cpu.A - dato - (1 - cpu.C);
        cpu.A = res;
        SetFlags(cpu, res, dato, oldA);
    } else {
        if (cpu.cycleAccurate) {
            cpu.remainingCycles++;
        }
        Word binaryRes = cpu.A - dato - (1 - cpu.C);
        Word res = SBC_Decimal(cpu, dato);
        cpu.A = res;
        SetFlagsBCD(cpu, binaryRes, dato, oldA);
    }
}

template <bool Debug>
inline void SBC::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);
    DoSBC(cpu, dato);
}

template <bool Debug>
inline void SBC::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    DoSBC(cpu, dato);
}

template <bool Debug>
inline void SBC::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    DoSBC(cpu, dato);
}

template <bool Debug>
inline void SBC::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    DoSBC(cpu, dato);
}

template <bool Debug>
inline void SBC::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;
    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);
    Byte dato = cpu.ReadByte<Debug>(effectiveAddr, bus);
    DoSBC(cpu, dato);
}

template <bool Debug>
inline void SBC::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.Y;
    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);
    Byte dato = cpu.ReadByte<Debug>(effectiveAddr, bus);
    DoSBC(cpu, dato);
}

template <bool Debug>
inline void SBC::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Word Dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    DoSBC(cpu, dato);
}

template <bool Debug>
inline void SBC::ExecuteINDY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Word baseAddr = cpu.ReadWordZP<Debug>(ZP_Dir, bus);
    Word effectiveAddr = baseAddr + cpu.Y;
    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);
    Byte dato = cpu.ReadByte<Debug>(effectiveAddr, bus);
    DoSBC(cpu, dato);
}

template <bool Debug>
inline void SBC::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Word dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);
    Byte dato = cpu.ReadByte<Debug>(dir, bus);
    DoSBC(cpu, dato);
}

inline void CMP::SetFlags(CPU& cpu, Byte dato) {
    cpu.C = (cpu.A >= dato);
    cpu.Z = (cpu.A == dato);
    cpu.N = ((cpu.A - dato) & 0b10000000) > 0;
}

template <bool Debug>
inline void CMP::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);

    SetFlags(cpu, dato);
}

template <bool Debug>
inline void CMP::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void CMP::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void CMP::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void CMP::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void CMP::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void CMP::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;

    Word Dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void CMP::ExecuteINDY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word baseAddr = cpu.ReadWordZP<Debug>(ZP_Dir, bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Byte dato = cpu.ReadByte<Debug>(effectiveAddr, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void CMP::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Word dir = cpu.ReadWordZP<Debug>(ZP_Dir, bus);

    Byte dato = cpu.ReadByte<Debug>(dir, bus);
    SetFlags(cpu, dato);
}

inline void CPX::SetFlags(CPU& cpu, Byte dato) {
    cpu.C = (cpu.X >= dato);
    cpu.Z = (cpu.X == dato);
    cpu.N = ((cpu.X - dato) & 0b10000000) > 0;
}

template <bool Debug>
inline void CPX::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);

    SetFlags(cpu, dato);
}

template <bool Debug>
inline void CPX::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void CPX::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu, dato);
}

inline void CPY::SetFlags(CPU& cpu, Byte dato) {
    cpu.C = (cpu.Y >= dato);
    cpu.Z = (cpu.Y == dato);
    cpu.N = ((cpu.Y - dato) & 0b10000000) > 0;
}

template <bool Debug>
inline void CPY::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte<Debug>(bus);

    SetFlags(cpu, dato);
}

template <bool Debug>
inline void CPY::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);

    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void CPY::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);

    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    SetFlags(cpu, dato);
}

inline void INC::SetFlags(CPU& cpu, Byte val) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
}

template <bool Debug>
inline void INC::ExecuteAccumulator(CPU& cpu, Bus& bus) {
    cpu.A++;
    SetFlags(cpu, cpu.A);
}

template <bool Debug>
inline void INC::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    dato++;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void INC::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    dato++;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void INC::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    dato++;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void INC::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;
    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);
    Byte dato = cpu.ReadByte<Debug>(effectiveAddr, bus);
    dato++;
    bus.Write<Debug>(effectiveAddr, dato);
    SetFlags(cpu, dato);
}

inline void DEC::SetFlags(CPU& cpu, Byte val) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
}

template <bool Debug>
inline void DEC::ExecuteAccumulator(CPU& cpu, Bus& bus) {
    cpu.A--;
    SetFlags(cpu, cpu.A);
}

template <bool Debug>
inline void DEC::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    dato--;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void DEC::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte<Debug>(ZP_Dir, bus);
    dato--;
    bus.Write<Debug>(ZP_Dir, dato);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void DEC::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Byte dato = cpu.ReadByte<Debug>(Dir, bus);
    dato--;
    bus.Write<Debug>(Dir, dato);
    SetFlags(cpu, dato);
}

template <bool Debug>
inline void DEC::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord<Debug>(bus);
    Word effectiveAddr = baseAddr + cpu.X;
    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);
    Byte dato = cpu.ReadByte<Debug>(effectiveAddr, bus);
    dato--;
    bus.Write<Debug>(effectiveAddr, dato);
    SetFlags(cpu, dato);
}

}  // namespace Hardware::Instructions

