#include "RMB.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

template <bool Debug, Byte Bit>
static void ExecuteBit(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    Byte data = cpu.ReadByte<Debug>(ZP_Dir, bus);
    data &= static_cast<Byte>(~(1U << Bit));
    bus.Write<Debug>(ZP_Dir, data);
}

template <bool Debug>
void RMB::Execute0(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 0>(cpu, bus);
}

template <bool Debug>
void RMB::Execute1(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 1>(cpu, bus);
}

template <bool Debug>
void RMB::Execute2(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 2>(cpu, bus);
}

template <bool Debug>
void RMB::Execute3(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 3>(cpu, bus);
}

template <bool Debug>
void RMB::Execute4(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 4>(cpu, bus);
}

template <bool Debug>
void RMB::Execute5(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 5>(cpu, bus);
}

template <bool Debug>
void RMB::Execute6(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 6>(cpu, bus);
}

template <bool Debug>
void RMB::Execute7(CPU& cpu, Bus& bus) {
    ExecuteBit<Debug, 7>(cpu, bus);
}

template void RMB::Execute0<true>(CPU&, Bus&);
template void RMB::Execute0<false>(CPU&, Bus&);
template void RMB::Execute1<true>(CPU&, Bus&);
template void RMB::Execute1<false>(CPU&, Bus&);
template void RMB::Execute2<true>(CPU&, Bus&);
template void RMB::Execute2<false>(CPU&, Bus&);
template void RMB::Execute3<true>(CPU&, Bus&);
template void RMB::Execute3<false>(CPU&, Bus&);
template void RMB::Execute4<true>(CPU&, Bus&);
template void RMB::Execute4<false>(CPU&, Bus&);
template void RMB::Execute5<true>(CPU&, Bus&);
template void RMB::Execute5<false>(CPU&, Bus&);
template void RMB::Execute6<true>(CPU&, Bus&);
template void RMB::Execute6<false>(CPU&, Bus&);
template void RMB::Execute7<true>(CPU&, Bus&);
template void RMB::Execute7<false>(CPU&, Bus&);

}  // namespace Hardware::Instructions
