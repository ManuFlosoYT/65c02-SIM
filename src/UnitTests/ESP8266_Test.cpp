#include "Hardware/Comm/ESP8266/ESP8266.h"

#include <gtest/gtest.h>

#include "Hardware/Core/Bus.h"
#include "Hardware/Core/Emulator/Emulator.h"
#include "Hardware/Memory/RAM.h"

using namespace Hardware;

class ESP8266_Test : public ::testing::Test {
   public:
    Bus bus;
    RAM ram{0x10000};
    ESP8266 esp;

    void SetUp() override {
        bus.RegisterDevice(0x0000, 0xFFFF, &ram, true, false);
        bus.RegisterDevice(0x5004, 0x5007, &esp, true, true);
        esp.Reset();
    }

    void SendString(const std::string& str) {
        for (char c : str) {
            bus.Write(0x5004, static_cast<Byte>(c));
        }
    }

    std::string ReadResponse() {
        std::string res;
        while ((bus.Read(0x5005) & 0x80) != 0) {
            res += static_cast<char>(bus.Read(0x5004));
        }
        return res;
    }
};

TEST_F(ESP8266_Test, ATCommandAndSnifferLog) {
    SendString("AT\r\n");
    std::string resp = ReadResponse();
    EXPECT_NE(resp.find("OK"), std::string::npos);

    auto log = esp.GetTrafficLog();
    EXPECT_GE(log.size(), 2U);

    bool foundTx = false;
    bool foundRx = false;
    for (const auto& pkt : log) {
        if (pkt.direction == TrafficDirection::TX && pkt.tag == "AT") {
            foundTx = true;
            EXPECT_EQ(pkt.channel, TrafficChannel::ATCommand);
        }
        if (pkt.direction == TrafficDirection::RX && pkt.payload.find("OK") != std::string::npos) {
            foundRx = true;
            EXPECT_EQ(pkt.channel, TrafficChannel::ATResponse);
        }
    }
    EXPECT_TRUE(foundTx);
    EXPECT_TRUE(foundRx);
}

TEST_F(ESP8266_Test, CWJAPCommand) {
    SendString("AT+CWJAP=\"TestNet\",\"testpass\"\r\n");
    std::string resp = ReadResponse();
    EXPECT_NE(resp.find("OK"), std::string::npos);

    auto log = esp.GetTrafficLog();
    bool foundJap = false;
    for (const auto& pkt : log) {
        if (pkt.direction == TrafficDirection::TX && pkt.payload.find("CWJAP") != std::string::npos) {
            foundJap = true;
            break;
        }
    }
    EXPECT_TRUE(foundJap);
}

TEST_F(ESP8266_Test, SnifferPauseAndClear) {
    SendString("AT\r\n");
    ReadResponse();
    EXPECT_FALSE(esp.GetTrafficLog().empty());

    esp.ClearTrafficLog();
    EXPECT_TRUE(esp.GetTrafficLog().empty());

    esp.SetSnifferPaused(true);
    EXPECT_TRUE(esp.IsSnifferPaused());

    SendString("AT\r\n");
    ReadResponse();
    EXPECT_TRUE(esp.GetTrafficLog().empty());

    esp.SetSnifferPaused(false);
    SendString("AT\r\n");
    ReadResponse();
    EXPECT_FALSE(esp.GetTrafficLog().empty());
}

TEST_F(ESP8266_Test, ACIAESPBridge) {
    Core::Emulator emu;
    emu.SetESPEnabled(true);
    emu.SetACIAESPBridge(true);
    EXPECT_TRUE(emu.IsACIAESPBridge());

    // When bridge is active, ACIA output callback forwards to ESP8266
    bool callbackFired = false;
    emu.SetOutputCallback([&](char c) {
        if (c == 'T') {
            callbackFired = true;
        }
    });

    auto& espRef = emu.GetESP8266();
    espRef.ClearTrafficLog();

    // Configure ACIA to enable transmitter at fast baud
    emu.GetMem().Write(0x5002, 0x01);
    emu.GetMem().Write(0x5003, 0x00);

    // Write and clock ACIA
    for (char c : std::string("AT\r\n")) {
        emu.GetMem().Write(0x5000, static_cast<Byte>(c));
        for (int i = 0; i < 1000; ++i) {
            emu.GetACIA().Clock(1000000);
        }
    }

    EXPECT_TRUE(callbackFired);

    auto log = espRef.GetTrafficLog();
    bool foundAciaTx = false;
    for (const auto& pkt : log) {
        if (pkt.tag == "ACIA -> ESP8266") {
            foundAciaTx = true;
            break;
        }
    }
    EXPECT_TRUE(foundAciaTx);
}

TEST_F(ESP8266_Test, HTTPClientCommandSyntax) {
    esp.ClearTrafficLog();
    SendString("AT+HTTPCLIENT=2,0,\"https://httpbin.org/get\"\r\n");

    auto log = esp.GetTrafficLog();
    bool foundCmd = false;
    for (const auto& pkt : log) {
        if (pkt.direction == TrafficDirection::TX && pkt.payload.find("AT+HTTPCLIENT") != std::string::npos) {
            foundCmd = true;
            break;
        }
    }
    EXPECT_TRUE(foundCmd);
}
