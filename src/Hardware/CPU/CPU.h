#pragma once
#include <cstdint>
#include <iostream>


#include "Hardware/Core/Bus.h"
#include "Hardware/Core/ISerializable.h"
#include "Hardware/Core/SubroutineProfiler.h"
#include "Hardware/CPU/CPURegisters.h"

using Byte = uint8_t;
using Word = uint16_t;

namespace Hardware {

class CPU;
class SubroutineProfiler;
namespace CPUDispatch {
template <bool Debug>
inline int Dispatch(CPU& cpu, Bus& bus);
}

class CPU : public CPURegisters, public ISerializable {
   public:
    // RETURNS:
    // 0: OK
    // 1: STOP/JAM
    // -1: Opcode no válido

    const Byte* current_page_ptr = nullptr;

    bool isInit = false;
    bool waiting = false;
    bool pendingNMI = false;

    int remainingCycles = 0;
    bool cycleAccurate = true;
    uint64_t cycleCount = 0;
    SubroutineProfiler* profiler = nullptr;

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& inputStream) override;

    void Reset();

    int Execute(Bus& bus);

    template <bool Debug>
    int Step(Bus& bus);
    template <bool Debug>
    int StepCycleAccurate(Bus& bus);
    template <bool Debug>
    int StepFast(Bus& bus);

    template <bool Debug = false>
    void EnsureInit(Bus& bus) {
        if (!isInit) {
            PC = ReadWord<Debug>(0xFFFC, bus);
            UpdatePagePtr(bus);
            isInit = true;
        } else if (current_page_ptr == nullptr) {
            UpdatePagePtr(bus);
        }
    }

    template <bool Debug>
    void IRQ(Bus& bus);
    void IRQ(Bus& bus);

    template <bool Debug>
    void NMI(Bus& bus);

    template <bool Debug>
    int Dispatch(Bus& bus);

    [[nodiscard]] bool IsCycleAccurate() const;
    void SetCycleAccurate(bool enabled);
    [[nodiscard]] int GetRemainingCycles() const;

    void SetProfiler(SubroutineProfiler* prof) { profiler = prof; }
    [[nodiscard]] SubroutineProfiler* GetProfiler() const { return profiler; }
    [[nodiscard]] uint64_t GetCycleCount() const { return cycleCount; }

    template <bool Debug>
    inline void TraceJSR(Word target, Word returnAddr) {
        if constexpr (Debug) {
            if (profiler != nullptr) {
                profiler->OnJSR(target, returnAddr, cycleCount);
            }
        }
    }
    template <bool Debug>
    inline void TraceRTS() {
        if constexpr (Debug) {
            if (profiler != nullptr) {
                profiler->OnRTS(cycleCount);
            }
        }
    }
    template <bool Debug>
    inline void TraceBRK(Word target, Word returnAddr) {
        if constexpr (Debug) {
            if (profiler != nullptr) {
                profiler->OnBRK(target, returnAddr, cycleCount);
            }
        }
    }
    template <bool Debug>
    inline void TraceRTI() {
        if constexpr (Debug) {
            if (profiler != nullptr) {
                profiler->OnRTI(cycleCount);
            }
        }
    }

    [[nodiscard]] bool HasPendingNMI() const { return pendingNMI; }
    void TriggerNMI() { pendingNMI = true; }
    void ClearPendingNMI() { pendingNMI = false; }

    void AddPageCrossPenalty(Word baseAddr, Word effectiveAddr);

    inline void UpdatePagePtr(Bus& bus);

    template <bool Debug>
    Byte FetchOpcode(Bus& bus);
    template <bool Debug>
    Byte FetchByte(Bus& bus);
    template <bool Debug>
    Word FetchWord(Bus& bus);
    template <bool Debug>
    Byte ReadByte(Word addr, Bus& bus);
    template <bool Debug>
    Word ReadWord(Word addr, Bus& bus);
    template <bool Debug>
    Word ReadWordZP(Byte zpAddr, Bus& bus);

    template <bool Debug>
    void PushByte(Byte val, Bus& bus);
    template <bool Debug>
    Byte PopByte(Bus& bus);
    template <bool Debug>
    void PushWord(Word val, Bus& bus);
    template <bool Debug>
    Word PopWord(Bus& bus);
};

}  // namespace Hardware

#include "Hardware/CPU/Dispatch.h"

inline void Hardware::CPU::Reset() {
    CPURegisters::Reset();
    isInit = false;
    waiting = false;
    pendingNMI = false;
    remainingCycles = 0;
    cycleCount = 0;
    if (profiler != nullptr) {
        profiler->Reset();
    }
}

inline int Hardware::CPU::Execute(Bus& bus) {
    if (bus.HasActiveHooks()) {
        while (true) {
            int res = Step<true>(bus);
            if (res != 0) {
                return res;
            }
        }
    } else {
        while (true) {
            int res = Step<false>(bus);
            if (res != 0) {
                return res;
            }
        }
    }
    return 0;
}

template <bool Debug>
inline int Hardware::CPU::Step(Bus& bus) {
    if (cycleAccurate) {
        return StepCycleAccurate<Debug>(bus);
    }
    int res = StepFast<Debug>(bus);
    remainingCycles = 0;
    return res;
}

template <bool Debug>
inline int Hardware::CPU::StepCycleAccurate(Bus& bus) {
    if constexpr (Debug) {
        cycleCount++;
    }
    EnsureInit<Debug>(bus);

    if (waiting) [[unlikely]] {
        return 0;
    }

    if (remainingCycles > 0) [[likely]] {
        remainingCycles--;
        return 0;
    }

    return Dispatch<Debug>(bus);
}

template <bool Debug>
inline int Hardware::CPU::StepFast(Bus& bus) {
    EnsureInit<Debug>(bus);

    if (waiting) [[unlikely]] {
        return 0;
    }

    int cycles = Dispatch<Debug>(bus);
    if constexpr (Debug) {
        cycleCount += cycles;
    }
    return cycles;
}

template <bool Debug>
inline void Hardware::CPU::IRQ(Bus& bus) {
    waiting = false;
    if (!I) {
        if (cycleAccurate) { remainingCycles += 7; }
        if constexpr (Debug) {
            if (profiler != nullptr) {
                profiler->OnInterrupt(0xFFFE, cycleCount, "IRQ");
            }
        }
        PushWord<Debug>(PC, bus);
        B = 0;
        PushByte<Debug>(GetStatus(), bus);
        I = 1;
        D = 0;
        PC = ReadWord<Debug>(0xFFFE, bus);
        UpdatePagePtr(bus);
    }
}

template <bool Debug>
inline void Hardware::CPU::NMI(Bus& bus) {
    if (cycleAccurate) { remainingCycles += 7; }
    if constexpr (Debug) {
        if (profiler != nullptr) {
            profiler->OnInterrupt(0xFFFA, cycleCount, "NMI");
        }
    }
    waiting = false;
    pendingNMI = false;
    PushWord<Debug>(PC, bus);
    B = 0;
    PushByte<Debug>(GetStatus(), bus);
    I = 1;
    D = 0;
    PC = ReadWord<Debug>(0xFFFA, bus);
    UpdatePagePtr(bus);
}

template <bool Debug>
inline int Hardware::CPU::Dispatch(Bus& bus) {
    return CPUDispatch::Dispatch<Debug>(*this, bus);
}

inline bool Hardware::CPU::IsCycleAccurate() const { return cycleAccurate; }
inline void Hardware::CPU::SetCycleAccurate(bool enabled) { cycleAccurate = enabled; }
inline int Hardware::CPU::GetRemainingCycles() const { return remainingCycles; }

inline void Hardware::CPU::AddPageCrossPenalty(Word baseAddr, Word effectiveAddr) {
    if (cycleAccurate && ((baseAddr & 0xFF00) != (effectiveAddr & 0xFF00))) {
        remainingCycles++;
        if (profiler != nullptr && profiler->IsEnabled()) [[unlikely]] {
            profiler->OnPageCross(instructionPC, baseAddr, effectiveAddr);
        }
    }
}

inline void Hardware::CPU::UpdatePagePtr(Bus& bus) {
    Byte* memoryBase = bus.GetPageReadPtr(PC >> 8);
    if (memoryBase != nullptr) {
        current_page_ptr = memoryBase + (PC & 0xFF);
    } else {
        current_page_ptr = nullptr;
    }
}

template <bool Debug>
inline Hardware::Byte Hardware::CPU::FetchOpcode(Bus& bus) {
    instructionPC = PC;
    if constexpr (Debug) {
        bus.LogCDL(PC, CDL_OPCODE);
    }
    Byte dato = 0;
    if (current_page_ptr != nullptr) [[likely]] {
        dato = *current_page_ptr;
        current_page_ptr++;
    } else {
        dato = bus.Read<Debug>(PC);
    }
    PC++;
    if ((PC & 0xFF) == 0) {
        UpdatePagePtr(bus);
    }
    return dato;
}

template <bool Debug>
inline Hardware::Byte Hardware::CPU::FetchByte(Bus& bus) {
    if constexpr (Debug) {
        bus.LogCDL(PC, CDL_OPERAND);
    }
    Byte dato = 0;
    if (current_page_ptr != nullptr) [[likely]] {
        dato = *current_page_ptr;
        current_page_ptr++;
    } else {
        dato = bus.Read<Debug>(PC);
    }
    PC++;
    if ((PC & 0xFF) == 0) {
        UpdatePagePtr(bus);
    }
    return dato;
}

template <bool Debug>
inline Hardware::Word Hardware::CPU::FetchWord(Bus& bus) {
    if (current_page_ptr != nullptr && (PC & 0xFF) < 0xFE) [[likely]] {
        if constexpr (Debug) {
            bus.LogCDL(PC, CDL_OPERAND);
            bus.LogCDL(static_cast<Word>(PC + 1), CDL_OPERAND);
        }
        Word dato = static_cast<Word>(current_page_ptr[0]) | (static_cast<Word>(current_page_ptr[1]) << 8);
        current_page_ptr += 2;
        PC += 2;
        return dato;
    }
    Word dato = FetchByte<Debug>(bus);
    dato |= (FetchByte<Debug>(bus) << 8);
    return dato;
}

template <bool Debug>
inline Hardware::Byte Hardware::CPU::ReadByte(Word addr, Bus& bus) {
    if constexpr (Debug) {
        bus.LogCDL(addr, CDL_DATA);
    }
    return bus.Read<Debug>(addr);
}

template <bool Debug>
inline Hardware::Word Hardware::CPU::ReadWord(Word addr, Bus& bus) {
    if constexpr (Debug) {
        bus.LogCDL(addr, CDL_DATA);
        bus.LogCDL(static_cast<Word>(addr + 1), CDL_DATA);
    }
    if constexpr (!Debug) {
        if ((addr & 0xFF) != 0xFF) [[likely]] {
            if (const Byte* page = bus.GetPageReadPtr(addr >> 8)) [[likely]] {
                const Word offset = addr & 0xFF;
                return static_cast<Word>(page[offset]) | (static_cast<Word>(page[offset + 1]) << 8);
            }
        }
    }
    Word dato = bus.Read<Debug>(addr);
    dato |= (bus.Read<Debug>(addr + 1) << 8);
    return dato;
}

template <bool Debug>
inline Hardware::Word Hardware::CPU::ReadWordZP(Byte zpAddr, Bus& bus) {
    if constexpr (Debug) {
        bus.LogCDL(zpAddr, CDL_ZP_POINTER);
        bus.LogCDL(static_cast<Byte>(zpAddr + 1), CDL_ZP_POINTER);
    }
    if constexpr (!Debug) {
        if (zpAddr != 0xFF) [[likely]] {
            if (const Byte* page0 = bus.GetPageReadPtr(0)) [[likely]] {
                return static_cast<Word>(page0[zpAddr]) | (static_cast<Word>(page0[zpAddr + 1]) << 8);
            }
        }
    }
    Word low = bus.Read<Debug>(zpAddr);
    Word high = bus.Read<Debug>(static_cast<Byte>(zpAddr + 1));
    return low | (high << 8);
}

template <bool Debug>
inline void Hardware::CPU::PushByte(Byte val, Bus& bus) {
    Byte sp = static_cast<Byte>(SP);
    bus.Write<Debug>(static_cast<Word>(0x0100 | sp), val);
    SP = static_cast<Word>(0x0100 | static_cast<Byte>(sp - 1));
}

template <bool Debug>
inline Hardware::Byte Hardware::CPU::PopByte(Bus& bus) {
    Byte sp = static_cast<Byte>(SP);
    sp = static_cast<Byte>(sp + 1);
    SP = static_cast<Word>(0x0100 | sp);
    return bus.Read<Debug>(static_cast<Word>(0x0100 | sp));
}

template <bool Debug>
inline void Hardware::CPU::PushWord(Word val, Bus& bus) {
    PushByte<Debug>((val >> 8) & 0xFF, bus);
    PushByte<Debug>(val & 0xFF, bus);
}

template <bool Debug>
inline Hardware::Word Hardware::CPU::PopWord(Bus& bus) {
    Word Low = PopByte<Debug>(bus);
    Word High = PopByte<Debug>(bus);
    return (High << 8) | Low;
}

inline void Hardware::CPU::IRQ(Bus& bus) {
    if (bus.HasActiveHooks()) {
        IRQ<true>(bus);
        return;
    }
    IRQ<false>(bus);
}
