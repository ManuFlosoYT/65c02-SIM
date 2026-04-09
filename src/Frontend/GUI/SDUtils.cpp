#include "Frontend/GUI/SDUtils.h"

#include <array>
#include <fstream>
#include <string_view>
#include <cstdint>

#include "Hardware/Core/Emulator.h"

namespace GUI {

bool IsSDCardEnabled(Control::AppState& state) {
    auto& bus = state.emulator.GetMem();
    const auto& devices = bus.GetRegisteredDevices();
    for (const auto& reg : devices) {
        if (reg.device == static_cast<Hardware::IBusDevice*>(&state.emulator.GetSDCard())) {
            return reg.enabled;
        }
    }
    return false;
}

bool CreateFAT16Image(const std::string& path) {
    static constexpr uint32_t BYTES_PER_SECTOR = 512;
    static constexpr uint32_t SECTORS_PER_CLUSTER = 8;
    static constexpr uint32_t RESERVED_SECTORS = 4;
    static constexpr uint32_t NUM_FATS = 2;
    static constexpr uint32_t ROOT_ENTRIES = 512;
    static constexpr uint32_t TOTAL_SECTORS = 65536;
    static constexpr uint32_t FAT_SECTORS = 32;

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }

    std::string zero(BYTES_PER_SECTOR, '\0');
    for (uint32_t i = 0; i < TOTAL_SECTORS; i++) {
        file.write(zero.data(), BYTES_PER_SECTOR);
    }
    file.seekp(0, std::ios::beg);

    std::array<char, BYTES_PER_SECTOR> boot{};
    boot[0] = static_cast<char>(0xEB);
    boot[1] = static_cast<char>(0x3C);
    boot[2] = static_cast<char>(0x90);
    std::string_view oem = "MSDOS5.0";
    for (size_t i = 0; i < oem.length(); i++) {
        boot.at(3 + i) = static_cast<char>(oem.at(i));
    }
    boot[11] = static_cast<char>(0x00);
    boot[12] = static_cast<char>(0x02);
    boot[13] = static_cast<char>(SECTORS_PER_CLUSTER);
    boot[14] = static_cast<char>(RESERVED_SECTORS & 0xFF);
    boot[15] = static_cast<char>(RESERVED_SECTORS >> 8);
    boot[16] = static_cast<char>(NUM_FATS);
    boot[17] = static_cast<char>(ROOT_ENTRIES & 0xFF);
    boot[18] = static_cast<char>(ROOT_ENTRIES >> 8);
    boot[19] = static_cast<char>(0x00);
    boot[20] = static_cast<char>(0x00);
    boot[21] = static_cast<char>(0xF8);
    boot[22] = static_cast<char>(FAT_SECTORS & 0xFF);
    boot[23] = static_cast<char>(FAT_SECTORS >> 8);
    boot[24] = static_cast<char>(63);
    boot[25] = static_cast<char>(0);
    boot[26] = static_cast<char>(255);
    boot[27] = static_cast<char>(0);
    boot[28] = boot[29] = boot[30] = boot[31] = static_cast<char>(0);
    boot[32] = static_cast<char>(TOTAL_SECTORS & 0xFF);
    boot[33] = static_cast<char>((TOTAL_SECTORS >> 8) & 0xFF);
    boot[34] = static_cast<char>((TOTAL_SECTORS >> 16) & 0xFF);
    boot[35] = static_cast<char>((TOTAL_SECTORS >> 24) & 0xFF);
    boot[36] = static_cast<char>(0x80);
    boot[37] = static_cast<char>(0x00);
    boot[38] = static_cast<char>(0x29);
    boot[39] = static_cast<char>(0xDE);
    boot[40] = static_cast<char>(0xAD);
    boot[41] = static_cast<char>(0xBE);
    boot[42] = static_cast<char>(0xEF);
    std::string_view label = "SD IMAGE   ";
    for (size_t i = 0; i < label.length(); i++) {
        boot.at(43 + i) = static_cast<char>(label.at(i));
    }
    std::string_view fstype = "FAT16   ";
    for (size_t i = 0; i < fstype.length(); i++) {
        boot.at(54 + i) = static_cast<char>(fstype.at(i));
    }
    boot[510] = static_cast<char>(0x55);
    boot[511] = static_cast<char>(0xAA);
    file.write(boot.data(), BYTES_PER_SECTOR);

    std::array<char, BYTES_PER_SECTOR> fatSector{};
    fatSector[0] = static_cast<char>(0xF8);
    fatSector[1] = static_cast<char>(0xFF);
    fatSector[2] = static_cast<char>(0xFF);
    fatSector[3] = static_cast<char>(0xFF);

    file.seekp(static_cast<std::streamoff>(RESERVED_SECTORS) * BYTES_PER_SECTOR, std::ios::beg);
    file.write(fatSector.data(), BYTES_PER_SECTOR);
    std::array<char, BYTES_PER_SECTOR> zeroBuf{};
    for (uint32_t size = 1; size < FAT_SECTORS; size++) {
        file.write(zeroBuf.data(), BYTES_PER_SECTOR);
    }

    file.write(fatSector.data(), BYTES_PER_SECTOR);
    for (uint32_t size = 1; size < FAT_SECTORS; size++) {
        file.write(zeroBuf.data(), BYTES_PER_SECTOR);
    }

    return file.good();
}

}  // namespace GUI
