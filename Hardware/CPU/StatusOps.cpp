#include "StatusOps.h"

#include "../CPU.h"

namespace Hardware {
namespace CPUStatusOps {

const Byte GetStatus(const CPU& cpu) {
    Byte status = 0;
    status |= cpu.C;
    status |= cpu.Z << 1;
    status |= cpu.I << 2;
    status |= cpu.D << 3;
    status |= cpu.B << 4;
    status |= cpu.V << 6;
    status |= cpu.N << 7;
    return status;
}

void SetStatus(CPU& cpu, Byte status) {
    cpu.C = status & 0x01;
    cpu.Z = (status >> 1) & 0x01;
    cpu.I = (status >> 2) & 0x01;
    cpu.D = (status >> 3) & 0x01;
    cpu.B = (status >> 4) & 0x01;
    cpu.V = (status >> 6) & 0x01;
    cpu.N = (status >> 7) & 0x01;
}

}  // namespace CPUStatusOps
}  // namespace Hardware
