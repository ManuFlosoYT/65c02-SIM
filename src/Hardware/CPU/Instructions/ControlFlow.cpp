#include "Hardware/CPU/Instructions/ControlFlow.h"

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Core/SubroutineProfiler.h"

namespace Hardware::Instructions {

template <bool Debug>
void JSR::Execute(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord<Debug>(bus);
    Word RetDir = cpu.PC - 1;

    if constexpr (Debug) {
        if (cpu.GetProfiler() != nullptr) {
            cpu.GetProfiler()->OnJSR(Dir, RetDir, cpu.GetCycleCount());
        }
    }

    cpu.PushWord<Debug>(RetDir, bus);

    cpu.PC = Dir;
    cpu.UpdatePagePtr(bus);
}

template <bool Debug>
void RTS::Execute(CPU& cpu, Bus& bus) {
    if constexpr (Debug) {
        if (cpu.GetProfiler() != nullptr) {
            cpu.GetProfiler()->OnRTS(cpu.GetCycleCount());
        }
    }
    Word dir = cpu.PopWord<Debug>(bus) + 1;
    cpu.PC = dir;
    cpu.UpdatePagePtr(bus);
}

template <bool Debug>
void BRK::Execute(CPU& cpu, Bus& bus) {
    cpu.B = true;
    Word returnPc = cpu.PC + 1;
    cpu.PushWord<Debug>(returnPc, bus);
    Byte processorStatus = cpu.GetStatus();
    cpu.PushByte<Debug>(processorStatus, bus);
    cpu.I = 1;
    cpu.D = 0;
    Word vectorAddr = 0xFFFE;
    if (cpu.HasPendingNMI()) {
        vectorAddr = 0xFFFA;
        cpu.ClearPendingNMI();
    }
    cpu.PC = cpu.ReadWord<Debug>(vectorAddr, bus);
    cpu.UpdatePagePtr(bus);

    if constexpr (Debug) {
        if (cpu.GetProfiler() != nullptr) {
            cpu.GetProfiler()->OnBRK(cpu.PC, returnPc, cpu.GetCycleCount());
        }
    }
}

template <bool Debug>
void RTI::Execute(CPU& cpu, Bus& bus) {
    if constexpr (Debug) {
        if (cpu.GetProfiler() != nullptr) {
            cpu.GetProfiler()->OnRTI(cpu.GetCycleCount());
        }
    }
    Byte status = cpu.PopByte<Debug>(bus);
    cpu.SetStatus(status);
    cpu.PC = cpu.PopWord<Debug>(bus);
    cpu.UpdatePagePtr(bus);
}

template void JSR::Execute<true>(CPU&, Bus&);
template void JSR::Execute<false>(CPU&, Bus&);
template void RTS::Execute<true>(CPU&, Bus&);
template void RTS::Execute<false>(CPU&, Bus&);
template void BRK::Execute<true>(CPU&, Bus&);
template void BRK::Execute<false>(CPU&, Bus&);
template void RTI::Execute<true>(CPU&, Bus&);
template void RTI::Execute<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions

