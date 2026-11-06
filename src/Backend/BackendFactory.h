#pragma once

#include <memory>
#include "Backend/IEmulatorBackend.h"

namespace Backend {

std::unique_ptr<IEmulatorBackend> CreateEmulatorBackend();

}  // namespace Backend
