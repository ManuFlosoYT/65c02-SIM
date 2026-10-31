#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>

namespace Hardware {

enum class PS2LinkState : uint8_t {
    PowerOnSelfTestAA = 0,
    PowerOnMouseId00,
    HostInhibitClk,
    HostRequestToSend,
    HostShiftF4,
    MouseAckFA,
    StreamMode
};

struct PS2MouseController {
    static constexpr uint8_t MAX_X = 99U;
    static constexpr uint8_t MAX_Y = 74U;
    static constexpr uint32_t INHIBIT_CYCLES = 100U;

    [[nodiscard]] static constexpr uint16_t EncodeFrame(uint8_t data) noexcept {
        uint16_t parity = ((std::popcount(data) & 1) == 0) ? 1U : 0U;
        return static_cast<uint16_t>((static_cast<uint16_t>(data) << 1U) | (parity << 9U) | (1U << 10U));
    }

    [[nodiscard]] static constexpr bool DecodeFrame(uint16_t frame, uint8_t& outByte) noexcept {
        if ((frame & 0x01U) != 0U) {
            return false;
        }
        if (((frame >> 10U) & 0x01U) != 1U) {
            return false;
        }
        auto dataAndParity = static_cast<unsigned int>((frame >> 1U) & 0x1FFU);
        if ((std::popcount(dataAndParity) & 1) != 1) {
            return false;
        }
        outByte = static_cast<uint8_t>((frame >> 1U) & 0xFFU);
        return true;
    }

    bool fpgaClkOut = true;
    bool fpgaDataOut = true;
    bool mouseClkOut = true;
    bool mouseDataOut = true;
    bool prevPs2Clk = true;

    PS2LinkState linkState = PS2LinkState::PowerOnSelfTestAA;
    uint32_t stateCounter = 0U;
    uint16_t shiftReg = 0U;
    uint8_t bitIndex = 0U;
    uint8_t lastRxByte = 0U;
    uint8_t lastHostTxByte = 0U;
    std::array<uint8_t, 3> packet{};
    uint8_t packetIndex = 0U;
    uint8_t posX = 0U;
    uint8_t posY = 0U;
    uint8_t buttons = 0U;
    bool mouseDirtyFlag = false;

    [[nodiscard]] constexpr bool GetPS2Clk() const noexcept { return fpgaClkOut && mouseClkOut; }
    [[nodiscard]] constexpr bool GetPS2Data() const noexcept { return fpgaDataOut && mouseDataOut; }

    constexpr void Reset() noexcept {
        fpgaClkOut = true;
        fpgaDataOut = true;
        mouseClkOut = true;
        mouseDataOut = true;
        prevPs2Clk = true;
        linkState = PS2LinkState::PowerOnSelfTestAA;
        stateCounter = 0U;
        shiftReg = 0U;
        bitIndex = 0U;
        lastRxByte = 0U;
        lastHostTxByte = 0U;
        packet.fill(0U);
        packetIndex = 0U;
        posX = 0U;
        posY = 0U;
        buttons = 0U;
        mouseDirtyFlag = false;
    }

    constexpr void SampleFallingEdge() noexcept {
        bool currentClk = GetPS2Clk();
        bool fallingEdge = prevPs2Clk && !currentClk;
        prevPs2Clk = currentClk;
        if (!fallingEdge) {
            return;
        }
        if (GetPS2Data()) {
            shiftReg = static_cast<uint16_t>(shiftReg | (1U << bitIndex));
        } else {
            shiftReg = static_cast<uint16_t>(shiftReg & ~(1U << bitIndex));
        }
        ++bitIndex;
        if (bitIndex == 11U) {
            bitIndex = 0U;
            uint8_t decoded = 0U;
            if (DecodeFrame(shiftReg, decoded)) {
                lastRxByte = decoded;
                if (linkState == PS2LinkState::StreamMode) {
                    ProcessPacketByte(lastRxByte);
                }
            }
        }
    }

    constexpr void ReceiveFrame(uint16_t frame) noexcept {
        fpgaClkOut = true;
        fpgaDataOut = true;
        bitIndex = 0U;
        shiftReg = 0U;
        for (uint8_t b = 0U; b < 11U; ++b) {
            mouseDataOut = ((frame >> b) & 0x01U) != 0U;
            mouseClkOut = true;
            prevPs2Clk = GetPS2Clk();
            mouseClkOut = false;
            SampleFallingEdge();
            mouseClkOut = true;
            prevPs2Clk = GetPS2Clk();
        }
        mouseDataOut = true;
    }

    constexpr void ReceiveByte(uint8_t data) noexcept {
        ReceiveFrame(EncodeFrame(data));
    }

    constexpr void ProcessPacketByte(uint8_t b) noexcept {
        if (packetIndex == 0U) {
            if ((b & 0x08U) == 0U) {
                return;
            }
            packet.at(0) = b;
            packetIndex = 1U;
            return;
        }
        if (packetIndex == 1U) {
            packet.at(1) = b;
            packetIndex = 2U;
            return;
        }
        packet.at(2) = b;
        packetIndex = 0U;

        uint8_t status = packet.at(0);
        buttons = static_cast<uint8_t>(status & 0x07U);
        int16_t dx = ((status & 0x10U) != 0U)
                         ? static_cast<int16_t>(static_cast<int16_t>(packet.at(1)) - 256)
                         : static_cast<int16_t>(packet.at(1));
        if ((status & 0x40U) != 0U) {
            dx = ((status & 0x10U) != 0U) ? -255 : 255;
        }
        int16_t dy = ((status & 0x20U) != 0U)
                         ? static_cast<int16_t>(static_cast<int16_t>(packet.at(2)) - 256)
                         : static_cast<int16_t>(packet.at(2));
        if ((status & 0x80U) != 0U) {
            dy = ((status & 0x20U) != 0U) ? -255 : 255;
        }
        posX = static_cast<uint8_t>(
            std::clamp<int16_t>(static_cast<int16_t>(posX) + dx, 0, static_cast<int16_t>(MAX_X)));
        posY = static_cast<uint8_t>(
            std::clamp<int16_t>(static_cast<int16_t>(posY) - dy, 0, static_cast<int16_t>(MAX_Y)));
        mouseDirtyFlag = true;
    }

    constexpr void Clock() noexcept {
        switch (linkState) {
            case PS2LinkState::PowerOnSelfTestAA:
                ReceiveByte(0xAAU);
                linkState = PS2LinkState::PowerOnMouseId00;
                break;
            case PS2LinkState::PowerOnMouseId00:
                ReceiveByte(0x00U);
                linkState = PS2LinkState::HostInhibitClk;
                stateCounter = 0U;
                fpgaClkOut = false;
                fpgaDataOut = true;
                prevPs2Clk = GetPS2Clk();
                break;
            case PS2LinkState::HostInhibitClk:
                fpgaClkOut = false;
                ++stateCounter;
                if (stateCounter >= INHIBIT_CYCLES) {
                    linkState = PS2LinkState::HostRequestToSend;
                    stateCounter = 0U;
                }
                break;
            case PS2LinkState::HostRequestToSend:
                fpgaDataOut = false;
                fpgaClkOut = true;
                prevPs2Clk = GetPS2Clk();
                shiftReg = EncodeFrame(0xF4U);
                bitIndex = 1U;
                linkState = PS2LinkState::HostShiftF4;
                break;
            case PS2LinkState::HostShiftF4: {
                uint16_t rxFrame = 0U;
                for (uint8_t b = 0U; b < 11U; ++b) {
                    mouseClkOut = true;
                    prevPs2Clk = GetPS2Clk();
                    mouseClkOut = false;
                    prevPs2Clk = GetPS2Clk();
                    if (GetPS2Data()) {
                        rxFrame = static_cast<uint16_t>(rxFrame | (1U << b));
                    }
                    if (bitIndex < 11U) {
                        fpgaDataOut = ((shiftReg >> bitIndex) & 0x01U) != 0U;
                        ++bitIndex;
                    } else {
                        fpgaDataOut = true;
                    }
                }
                mouseDataOut = false;
                mouseClkOut = false;
                uint8_t txDecoded = 0U;
                if (DecodeFrame(rxFrame, txDecoded)) {
                    lastHostTxByte = txDecoded;
                }
                fpgaDataOut = true;
                mouseDataOut = true;
                mouseClkOut = true;
                prevPs2Clk = GetPS2Clk();
                bitIndex = 0U;
                linkState = PS2LinkState::MouseAckFA;
                break;
            }
            case PS2LinkState::MouseAckFA:
                ReceiveByte(0xFAU);
                linkState = PS2LinkState::StreamMode;
                break;
            case PS2LinkState::StreamMode:
                break;
        }
    }

    constexpr void CompleteInit() noexcept {
        while (linkState != PS2LinkState::StreamMode) {
            Clock();
        }
    }

    constexpr void InjectPacket(int16_t dx, int16_t dy, uint8_t btns) noexcept {
        if (linkState != PS2LinkState::StreamMode) {
            CompleteInit();
        }
        auto status = static_cast<uint8_t>((btns & 0x07U) | 0x08U);
        if (dx < 0) {
            status = static_cast<uint8_t>(status | 0x10U);
        }
        if (dy < 0) {
            status = static_cast<uint8_t>(status | 0x20U);
        }
        if (dx > 255 || dx < -255) {
            status = static_cast<uint8_t>(status | 0x40U);
            dx = std::clamp<int16_t>(dx, -255, 255);
        }
        if (dy > 255 || dy < -255) {
            status = static_cast<uint8_t>(status | 0x80U);
            dy = std::clamp<int16_t>(dy, -255, 255);
        }
        auto b1 = static_cast<uint8_t>(static_cast<uint16_t>(dx) & 0xFFU);
        auto b2 = static_cast<uint8_t>(static_cast<uint16_t>(dy) & 0xFFU);
        ReceiveByte(status);
        ReceiveByte(b1);
        ReceiveByte(b2);
    }

    [[nodiscard]] constexpr PS2LinkState GetLinkState() const noexcept { return linkState; }
    [[nodiscard]] constexpr bool IsStreamMode() const noexcept { return linkState == PS2LinkState::StreamMode; }
    [[nodiscard]] constexpr uint8_t GetPosX() const noexcept { return posX; }
    [[nodiscard]] constexpr uint8_t GetPosY() const noexcept { return posY; }
    [[nodiscard]] constexpr uint8_t GetButtons() const noexcept { return buttons; }
    [[nodiscard]] constexpr bool IsDirty() const noexcept { return mouseDirtyFlag; }
    constexpr void ClearDirty() noexcept { mouseDirtyFlag = false; }
    [[nodiscard]] constexpr uint8_t GetLastRxByte() const noexcept { return lastRxByte; }
    [[nodiscard]] constexpr uint8_t GetLastHostTxByte() const noexcept { return lastHostTxByte; }
    [[nodiscard]] constexpr uint8_t GetPacketIndex() const noexcept { return packetIndex; }
};

}  // namespace Hardware
