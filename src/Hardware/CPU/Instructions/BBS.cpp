#include "BBS.h"    
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

template <bool Debug, Byte Mask>
static void ExecuteBBS(CPU& cpu, Bus& bus) {
    Byte zpAddr = cpu.FetchByte<Debug>(bus);
    int8_t offset = cpu.FetchByte<Debug>(bus);
    Word target = static_cast<Word>(cpu.PC + offset);
    Byte val = cpu.ReadByte<Debug>(zpAddr, bus);

    if ((val & Mask) != 0) {
        cpu.remainingCycles++;
        cpu.AddPageCrossPenalty(cpu.PC, target);
        cpu.PC = target;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
void BBS::Execute0(CPU& cpu, Bus& bus) {
    ExecuteBBS<Debug, 0b00000001>(cpu, bus);
}

template <bool Debug>
void BBS::Execute1(CPU& cpu, Bus& bus) {
    ExecuteBBS<Debug, 0b00000010>(cpu, bus);
}

template <bool Debug>
void BBS::Execute2(CPU& cpu, Bus& bus) {
    ExecuteBBS<Debug, 0b00000100>(cpu, bus);
}

template <bool Debug>
void BBS::Execute3(CPU& cpu, Bus& bus) {
    ExecuteBBS<Debug, 0b00001000>(cpu, bus);
}

template <bool Debug>
void BBS::Execute4(CPU& cpu, Bus& bus) {
    ExecuteBBS<Debug, 0b00010000>(cpu, bus);
}

template <bool Debug>
void BBS::Execute5(CPU& cpu, Bus& bus) {
    ExecuteBBS<Debug, 0b00100000>(cpu, bus);
}

template <bool Debug>
void BBS::Execute6(CPU& cpu, Bus& bus) {
    ExecuteBBS<Debug, 0b01000000>(cpu, bus);
}

template <bool Debug>
void BBS::Execute7(CPU& cpu, Bus& bus) {
    ExecuteBBS<Debug, 0b10000000>(cpu, bus);
}

template void BBS::Execute0<true>(CPU&, Bus&);
template void BBS::Execute0<false>(CPU&, Bus&);
template void BBS::Execute1<true>(CPU&, Bus&);
template void BBS::Execute1<false>(CPU&, Bus&);
template void BBS::Execute2<true>(CPU&, Bus&);
template void BBS::Execute2<false>(CPU&, Bus&);
template void BBS::Execute3<true>(CPU&, Bus&);
template void BBS::Execute3<false>(CPU&, Bus&);
template void BBS::Execute4<true>(CPU&, Bus&);
template void BBS::Execute4<false>(CPU&, Bus&);
template void BBS::Execute5<true>(CPU&, Bus&);
template void BBS::Execute5<false>(CPU&, Bus&);
template void BBS::Execute6<true>(CPU&, Bus&);
template void BBS::Execute6<false>(CPU&, Bus&);
template void BBS::Execute7<true>(CPU&, Bus&);
template void BBS::Execute7<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
