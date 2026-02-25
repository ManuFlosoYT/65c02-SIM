#include "Hardware/CPU/CPU.h"

namespace Hardware {

bool CPU::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&PC), sizeof(PC));
    out.write(reinterpret_cast<const char*>(&SP), sizeof(SP));
    out.write(reinterpret_cast<const char*>(&A), sizeof(A));
    out.write(reinterpret_cast<const char*>(&X), sizeof(X));
    out.write(reinterpret_cast<const char*>(&Y), sizeof(Y));

    Byte status = GetStatus();
    out.write(reinterpret_cast<const char*>(&status), sizeof(status));

    out.write(reinterpret_cast<const char*>(&isInit), sizeof(isInit));
    out.write(reinterpret_cast<const char*>(&waiting), sizeof(waiting));
    out.write(reinterpret_cast<const char*>(&remainingCycles),
              sizeof(remainingCycles));
    out.write(reinterpret_cast<const char*>(&cycleAccurate),
              sizeof(cycleAccurate));

    return out.good();
}

bool CPU::LoadState(std::istream& in) {
    in.read(reinterpret_cast<char*>(&PC), sizeof(PC));
    in.read(reinterpret_cast<char*>(&SP), sizeof(SP));
    in.read(reinterpret_cast<char*>(&A), sizeof(A));
    in.read(reinterpret_cast<char*>(&X), sizeof(X));
    in.read(reinterpret_cast<char*>(&Y), sizeof(Y));

    Byte status;
    in.read(reinterpret_cast<char*>(&status), sizeof(status));
    SetStatus(status);

    in.read(reinterpret_cast<char*>(&isInit), sizeof(isInit));
    in.read(reinterpret_cast<char*>(&waiting), sizeof(waiting));
    in.read(reinterpret_cast<char*>(&remainingCycles), sizeof(remainingCycles));
    in.read(reinterpret_cast<char*>(&cycleAccurate), sizeof(cycleAccurate));

    return in.good();
}

}  // namespace Hardware
