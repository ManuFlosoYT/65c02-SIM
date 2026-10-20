#include "Hardware/Core/CartridgeLoader.h"
#include "Hardware/Core/Emulator.h"
#include "Hardware/Comm/SDCard.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <miniz.h>
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

TEST(System_Cartridge_Test, ParseBus_VirtualLCD_NoOverlapError) {
    Cartridge cart;
    nlohmann::json manifest = {{"bus",
                                {{{"name", "RAM"}, {"start", 0x0000}, {"end", 0x7FFF}},
                                 {{"name", "ROM"}, {"start", 0x8000}, {"end", 0xFFFF}},
                                 {{"name", "VIA"}, {"start", 0x6000}, {"end", 0x600F}},
                                 {{"name", "LCD"}}}}};

    EXPECT_NO_THROW({ CartridgeLoader::ParseBus(manifest, cart); });

    EXPECT_EQ(cart.busDevices.size(), 4);
}

TEST(System_Cartridge_Test, CartridgeLayout_LCDRegisteredByDefault) {
    Emulator emulator;
    Cartridge cart;
    cart.loaded = true;
    cart.busDevices = {{"RAM", 0x0000, 0x7FFF}, {"ROM", 0x8000, 0xFFFF}, {"VIA", 0x6000, 0x600F}};
    cart.romData.resize(0x8000, 0xEA);

    emulator.SetCartridge(cart);
    emulator.Reset();

    bool foundLcd = false;
    for (const auto& dev : emulator.GetMem().GetRegisteredDevices()) {
        if (dev.device == &emulator.GetLCD() && dev.enabled) {
            foundLcd = true;
            break;
        }
    }
    EXPECT_TRUE(foundLcd);

    emulator.GetMem().Write(0x6002, 0xFF);
    emulator.GetMem().Write(0x6000, 0x02 | 0x40);
    emulator.GetMem().Write(0x6000, 0x02);
    EXPECT_TRUE(emulator.GetLCD().IsInitialized());
}

TEST(System_Cartridge_Test, Load_Debug_Symbols) {
    std::filesystem::path tempZipPath = std::filesystem::temp_directory_path() / "65c02sim_test_dbg.65c";

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    ASSERT_TRUE(mz_zip_writer_init_file(&zip, tempZipPath.string().c_str(), 0));

    nlohmann::json manifest = {
        {"version", "1.0"},
        {"rom", "rom.bin"},
        {"dbg", "rom.dbg"},
        {"bus", {
            {{"name", "RAM"}, {"start", 0x0000}, {"end", 0x7FFF}},
            {{"name", "ROM"}, {"start", 0x8000}, {"end", 0xFFFF}}
        }}
    };
    std::string manifestStr = manifest.dump();
    ASSERT_TRUE(mz_zip_writer_add_mem(&zip, "manifest.json", manifestStr.data(), manifestStr.size(), MZ_DEFAULT_COMPRESSION));

    std::vector<uint8_t> fakeRom(0x8000, 0xEA);
    ASSERT_TRUE(mz_zip_writer_add_mem(&zip, "rom.bin", fakeRom.data(), fakeRom.size(), MZ_DEFAULT_COMPRESSION));

    std::string fakeDbg = "DBGSYMBOLS_TEST_PAYLOAD";
    ASSERT_TRUE(mz_zip_writer_add_mem(&zip, "rom.dbg", fakeDbg.data(), fakeDbg.size(), MZ_DEFAULT_COMPRESSION));

    ASSERT_TRUE(mz_zip_writer_finalize_archive(&zip));
    ASSERT_TRUE(mz_zip_writer_end(&zip));

    Cartridge cart;
    std::string errorMsg;
    bool success = CartridgeLoader::Load(tempZipPath.string(), cart, errorMsg);
    EXPECT_TRUE(success) << errorMsg;
    EXPECT_TRUE(cart.loaded);
    EXPECT_EQ(cart.dbgFileName, "rom.dbg");
    EXPECT_EQ(cart.dbgData.size(), fakeDbg.size());
    std::string extractedContent(cart.dbgData.begin(), cart.dbgData.end());
    EXPECT_EQ(extractedContent, fakeDbg);
    EXPECT_FALSE(cart.dbgFilePath.empty());
    EXPECT_TRUE(std::filesystem::exists(cart.dbgFilePath));

    std::error_code ec;
    if (!cart.dbgFilePath.empty()) {
        std::filesystem::remove(cart.dbgFilePath, ec);
    }
    std::filesystem::remove(tempZipPath, ec);
}
