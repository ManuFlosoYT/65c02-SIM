#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Control {

struct AppState;

bool LoadBinaryRom(AppState& state, const std::string& path, const std::vector<uint8_t>& data = {});
bool LoadCartridge(AppState& state, const std::string& path, const std::vector<uint8_t>& data = {});

}  // namespace Control
