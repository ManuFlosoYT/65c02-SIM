#pragma once

#include <string>
#include "Hardware/Core/Cartridge.h"

namespace Core {

class CartridgeLoader {
public:
    static bool Load(const std::string& path, Cartridge& outCartridge, std::string& errorMsg);
};

} // namespace Core
