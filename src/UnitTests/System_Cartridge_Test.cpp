#include "Hardware/Core/CartridgeLoader.h"
#include "Hardware/Comm/SDCard.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace Core;

TEST(System_Cartridge_Test, ParseBus_ValidManifest) {
    Cartridge cart;
    nlohmann::json manifest = {
        {"bus", {
            {{"name", "RAM"}, {"start", 0x0000}, {"end", 0x7FFF}},
            {{"name", "ROM"}, {"start", 0x8000}, {"end", 0xFFFF}},
            {{"name", "VIA"}, {"start", 0x6000}, {"end", 0x600F}}
        }}
    };
    
    EXPECT_NO_THROW({
        CartridgeLoader::ParseBus(manifest, cart);
    });
    
    EXPECT_EQ(cart.busDevices.size(), 3);
}

TEST(System_Cartridge_Test, ParseBus_MissingRAM) {
    Cartridge cart;
    nlohmann::json manifest = {
        {"bus", {
            {{"name", "ROM"}, {"start", 0x8000}, {"end", 0xFFFF}}
        }}
    };
    
    EXPECT_THROW({
        CartridgeLoader::ParseBus(manifest, cart);
    }, std::runtime_error);
}

TEST(System_Cartridge_Test, ParseBus_InvalidDevice) {
    Cartridge cart;
    nlohmann::json manifest = {
        {"bus", {
            {{"name", "RAM"}, {"start", 0x0000}, {"end", 0x7FFF}},
            {{"name", "ROM"}, {"start", 0x8000}, {"end", 0xFFFF}},
            {{"name", "MAGIC_DEVICE"}, {"start", 0x1000}, {"end", 0x1FFF}}
        }}
    };
    
    EXPECT_THROW({
        CartridgeLoader::ParseBus(manifest, cart);
    }, std::runtime_error);
}

TEST(System_Cartridge_Test, ParseBus_InvalidRange) {
    Cartridge cart;
    nlohmann::json manifest = {
        {"bus", {
            {{"name", "RAM"}, {"start", 0x7FFF}, {"end", 0x0000}}, // start > end
            {{"name", "ROM"}, {"start", 0x8000}, {"end", 0xFFFF}}
        }}
    };
    
    EXPECT_THROW({
        CartridgeLoader::ParseBus(manifest, cart);
    }, std::runtime_error);
}

TEST(System_Cartridge_Test, SDCardModifiedTracking) {
    Hardware::SDCard sd;
    EXPECT_FALSE(sd.IsModified());

    std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "65c02sim_test_sd.img";
    {
        std::ofstream ofs(tempPath, std::ios::binary | std::ios::trunc);
        std::vector<char> zeroes(1024, 0);
        ofs.write(zeroes.data(), static_cast<std::streamsize>(zeroes.size()));
    }

    ASSERT_TRUE(sd.Mount(tempPath.string()));
    EXPECT_FALSE(sd.IsModified());

    sd.SetModified(true);
    EXPECT_TRUE(sd.IsModified());

    sd.SetModified(false);
    EXPECT_FALSE(sd.IsModified());

    sd.Unmount();
    EXPECT_FALSE(sd.IsModified());
    std::filesystem::remove(tempPath);
}
