#include <gtest/gtest.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "Hardware/Core/Emulator.h"

using namespace Core;

TEST(Microdos_Boot_Test, TraceBoot) {
    Emulator emulator;
    emulator.SetHeadless(true);
    emulator.SetGPUEnabled(false);

    std::string consoleOutput;
    emulator.SetOutputCallback([&](char c) {
        consoleOutput += c;
    });

    std::string error;
    bool ok = emulator.Init("output/rom/microDOS.bin", error);
    ASSERT_TRUE(ok) << "Failed to init ROM: " << error;

    std::map<uint16_t, int> pcCounts;
    uint16_t lastPC = 0;
    int irqCount = 0;

    for (int i = 0; i < 200000; ++i) {
        uint16_t pc = emulator.GetCPU().PC;
        if (pc == 0x8078) {
            irqCount++;
            if (irqCount <= 5) {
                std::cout << "IRQ #" << irqCount 
                          << " ACIA STATUS=0x" << std::hex << (int)emulator.GetACIA().GetStatus()
                          << " VIA IFR=0x" << (int)emulator.GetVIA().Read(0x600D, emulator.GetBus())
                          << std::dec << "\n";
            }
        }
        pcCounts[pc]++;
        lastPC = pc;
        if (emulator.IsHalted() || emulator.Step() != 0) {
            break;
        }
    }
    std::cout << "Total IRQ count: " << irqCount << "\n";

    std::cout << "Console Output: [" << consoleOutput << "]\n";
    std::cout << "Last PC: 0x" << std::hex << lastPC << std::dec << "\n";
    std::cout << "Top executed PCs:\n";
    std::vector<std::pair<uint16_t, int>> sortedPCs(pcCounts.begin(), pcCounts.end());
    std::sort(sortedPCs.begin(), sortedPCs.end(), [](auto& a, auto& b) { return a.second > b.second; });
    for (size_t i = 0; i < std::min<size_t>(10, sortedPCs.size()); ++i) {
        std::cout << "  0x" << std::hex << sortedPCs[i].first << ": " << std::dec << sortedPCs[i].second << " times\n";
    }
}

