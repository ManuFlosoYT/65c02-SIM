#include "Hardware/CPU/CPU.h"

namespace Hardware {

bool CPU::SaveState(std::ostream& out) const {
    Serialize(out, PC);
    Serialize(out, SP);
    Serialize(out, A);
    Serialize(out, X);
    Serialize(out, Y);
    Serialize(out, GetStatus());
    Serialize(out, isInit);
    Serialize(out, waiting);
    Serialize(out, remainingCycles);
    Serialize(out, cycleAccurate);
    return out.good();
}

bool CPU::LoadState(std::istream& inputStream) {
    Deserialize(inputStream, PC);
    Deserialize(inputStream, SP);
    Deserialize(inputStream, A);
    Deserialize(inputStream, X);
    Deserialize(inputStream, Y);

    Byte status = 0;
    Deserialize(inputStream, status);
    SetStatus(status);

    Deserialize(inputStream, isInit);
    Deserialize(inputStream, waiting);
    Deserialize(inputStream, remainingCycles);
    Deserialize(inputStream, cycleAccurate);

    current_page_ptr = nullptr;

    return inputStream.good();
}

}  // namespace Hardware
