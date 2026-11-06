#include "Backend/BackendFactory.h"
#include "Hardware/Core/Emulator/Emulator.h"

namespace Backend {

std::unique_ptr<IEmulatorBackend> CreateEmulatorBackend() {
    return std::make_unique<Core::Emulator>();
}

}  // namespace Backend
