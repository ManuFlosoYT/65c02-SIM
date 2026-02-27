#include "Hardware/CPU/CPU.h"

namespace Hardware {

bool CPU::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&PC), sizeof(PC));  // NOLINT
    out.write(reinterpret_cast<const char*>(&SP), sizeof(SP));  // NOLINT
    out.write(reinterpret_cast<const char*>(&A), sizeof(A));    // NOLINT
    out.write(reinterpret_cast<const char*>(&X), sizeof(X));    // NOLINT
    out.write(reinterpret_cast<const char*>(&Y), sizeof(Y));    // NOLINT

    Byte status = GetStatus();
    out.write(reinterpret_cast<const char*>(&status), sizeof(status));  // NOLINT

    out.write(reinterpret_cast<const char*>(&isInit), sizeof(isInit));    // NOLINT
    out.write(reinterpret_cast<const char*>(&waiting), sizeof(waiting));  // NOLINT
    out.write(reinterpret_cast<const char*>(&remainingCycles),            // NOLINT
              sizeof(remainingCycles));
    out.write(reinterpret_cast<const char*>(&cycleAccurate),  // NOLINT
              sizeof(cycleAccurate));

    return out.good();
}

bool CPU::LoadState(std::istream& inputStream) {
    inputStream.read(reinterpret_cast<char*>(&PC), sizeof(PC));  // NOLINT
    inputStream.read(reinterpret_cast<char*>(&SP), sizeof(SP));  // NOLINT
    inputStream.read(reinterpret_cast<char*>(&A), sizeof(A));    // NOLINT
    inputStream.read(reinterpret_cast<char*>(&X), sizeof(X));    // NOLINT
    inputStream.read(reinterpret_cast<char*>(&Y), sizeof(Y));    // NOLINT

    Byte status = 0;
    inputStream.read(reinterpret_cast<char*>(&status), sizeof(status));  // NOLINT
    SetStatus(status);

    inputStream.read(reinterpret_cast<char*>(&isInit), sizeof(isInit));                    // NOLINT
    inputStream.read(reinterpret_cast<char*>(&waiting), sizeof(waiting));                  // NOLINT
    inputStream.read(reinterpret_cast<char*>(&remainingCycles), sizeof(remainingCycles));  // NOLINT
    inputStream.read(reinterpret_cast<char*>(&cycleAccurate), sizeof(cycleAccurate));      // NOLINT

    return inputStream.good();
}

}  // namespace Hardware
