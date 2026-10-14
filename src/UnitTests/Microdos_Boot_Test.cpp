#include <gtest/gtest.h>

#include <string>

#include "Hardware/Core/CartridgeLoader.h"
#include "Hardware/Core/Emulator.h"

using namespace Core;

TEST(Microdos_Boot_Test, BootAndExecuteLS) {
    Emulator emulator;
    std::string consoleOutput;
    emulator.SetOutputCallback([&](char c) { consoleOutput += c; });

    std::string error;
    Core::Cartridge cart;
    bool ok = Core::CartridgeLoader::Load("output/cartridge/microDOS.65c", cart, error);
    ASSERT_TRUE(ok) << "Failed to load cartridge: " << error;
    emulator.SetCartridge(cart);
    emulator.SetHeadless(true);
    emulator.SetGPUEnabled(false);
    emulator.SetSDEnabled(true);
    ok = emulator.InitFromMemory(cart.romData, cart.romFileName, error);
    ASSERT_TRUE(ok) << "Failed to init ROM: " << error;
    if (!cart.sdCardPath.empty()) {
        emulator.GetSDCard().Mount(cart.sdCardPath);
    }

    bool injected = false;
    for (int i = 0; i < 2000000; ++i) {
        if (!injected && consoleOutput.find("> / > ") != std::string::npos) {
            injected = true;
            for (char c : std::string("ls\r")) {
                emulator.InjectKey(c);
            }
        }
        if (emulator.IsHalted() || emulator.Step() != 0) {
            break;
        }
    }

    EXPECT_TRUE(consoleOutput.find("Mounting SD Card...Ready") != std::string::npos);
    EXPECT_TRUE(consoleOutput.find("[D] BIN") != std::string::npos);
    EXPECT_TRUE(consoleOutput.find("[D] SID") != std::string::npos);
    EXPECT_TRUE(consoleOutput.find("[D] SYSTEM") != std::string::npos);
}

TEST(Microdos_Boot_Test, BootAndPlayAudio) {
    Emulator emulator;
    std::string consoleOutput;
    emulator.SetOutputCallback([&](char c) { consoleOutput += c; });

    std::string error;
    Core::Cartridge cart;
    bool ok = Core::CartridgeLoader::Load("output/cartridge/microDOS.65c", cart, error);
    ASSERT_TRUE(ok) << "Failed to load cartridge: " << error;
    emulator.SetCartridge(cart);
    emulator.SetHeadless(true);
    emulator.SetGPUEnabled(false);
    emulator.SetSDEnabled(true);
    emulator.SetSIDEnabled(true);
    ok = emulator.InitFromMemory(cart.romData, cart.romFileName, error);
    ASSERT_TRUE(ok) << "Failed to init ROM: " << error;
    if (!cart.sdCardPath.empty()) {
        emulator.GetSDCard().Mount(cart.sdCardPath);
    }

    bool injected = false;
    for (int i = 0; i < 3000000; ++i) {
        if (!injected && consoleOutput.find("> / > ") != std::string::npos) {
            injected = true;
            for (char c : std::string("play awake\r")) {
                emulator.InjectKey(c);
            }
        }
        if (emulator.IsHalted() || emulator.Step() != 0) {
            break;
        }
    }

    EXPECT_TRUE(consoleOutput.find("microDOS SID Player v2") != std::string::npos);
    EXPECT_TRUE(consoleOutput.find("Playing -> awake") != std::string::npos);
    EXPECT_TRUE(consoleOutput.find("Playback finished.") == std::string::npos);
}
