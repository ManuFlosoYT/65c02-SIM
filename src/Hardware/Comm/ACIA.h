#pragma once

#include <functional>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

// ACIA STATUS BITMASKS
constexpr Byte ACIA_STATUS_IRQ  = 0x80;
constexpr Byte ACIA_STATUS_DSR  = 0x40;
constexpr Byte ACIA_STATUS_DCD  = 0x20;
constexpr Byte ACIA_STATUS_TDRE = 0x10;
constexpr Byte ACIA_STATUS_RDRF = 0x08;
constexpr Byte ACIA_STATUS_OVRN = 0x04;
constexpr Byte ACIA_STATUS_FE   = 0x02;
constexpr Byte ACIA_STATUS_PE   = 0x01;

// ACIA CMD BITMASKS
constexpr Byte ACIA_CMD_ECHO    = 0x10;
constexpr Byte ACIA_CMD_RX_IRQ_DISABLE = 0x02;
constexpr Byte ACIA_CMD_DTR     = 0x01;

class ACIA : public IBusDevice {
   public:
    ACIA();
    void Reset() override;
    void ReceiveData(Byte data);

    // IBusDevice implementation
    inline Byte Read(Word address) override;
    inline Byte ReadDirect(Word address) override;
    void Write(Word address, Byte data) override;
    [[nodiscard]] inline std::string GetName() const override;

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& inStream) override;

    void SetOutputCallback(std::function<void(char)> callback);

    [[nodiscard]] bool HasIRQ() const { return (STATUS & ACIA_STATUS_IRQ) != 0; }
    [[nodiscard]] bool CanReceive() const;
    [[nodiscard]] int GetBaudRate() const;
    [[nodiscard]] int GetFrameBits() const;

    [[nodiscard]] inline bool GetTxPin() const { return !isShifting || ((shiftRegisterData & 0x01) != 0); }
    [[nodiscard]] inline bool GetRxPin() const { return true; }
    [[nodiscard]] inline Byte GetTxData() const { return txData; }
    [[nodiscard]] inline Byte GetRxData() const { return rxData; }

    inline void Clock(int ips = 1000000);
    [[nodiscard]] bool IsIdle() const;

   private:
    void ClockSlow(int ips);
    void UpdateIRQ();

    Byte txData;
    Byte rxData;
    Byte STATUS;
    Byte CMD;
    Byte CTRL;

    int shiftRegisterTicks;
    Byte shiftRegisterData;
    bool isShifting;
    bool echoPending;
    Byte echoByte;

    bool dcdState;
    bool dsrState;
    bool dcdInterrupt;
    bool dsrInterrupt;

    std::function<void(char)> outputCallback;
};

}  // namespace Hardware

#include "Hardware/Core/Bus.h"

namespace Hardware {

inline std::string ACIA::GetName() const { return "ACIA"; }

inline void ACIA::SetOutputCallback(std::function<void(char)> callback) { outputCallback = std::move(callback); }

inline Byte ACIA::Read(Word address) {
    switch (address & 0x03) {
        case ACIA_DATA & 0x03:
            STATUS &= ~ACIA_STATUS_RDRF;  // Clear interrupt/ready bit on read
            UpdateIRQ();
            return rxData;
        case ACIA_STATUS & 0x03: {
            Byte ret = STATUS;
            dcdInterrupt = false;
            dsrInterrupt = false;
            UpdateIRQ();
            return ret;
        }
        case ACIA_CMD & 0x03:
            return CMD;
        case ACIA_CTRL & 0x03:
            return CTRL;
        default:
            return 0;
    }
}

inline Byte ACIA::ReadDirect(Word address) {
    switch (address & 0x03) {
        case ACIA_DATA & 0x03:
            return rxData;
        case ACIA_STATUS & 0x03:
            return STATUS;
        case ACIA_CMD & 0x03:
            return CMD;
        case ACIA_CTRL & 0x03:
            return CTRL;
        default:
            return 0;
    }
}

inline bool ACIA::IsIdle() const {
    return !isShifting && !echoPending &&
           ((STATUS & ACIA_STATUS_TDRE) != 0 || (CMD & ACIA_CMD_DTR) == 0 || (CMD & ACIA_CMD_ECHO) != 0);
}

inline void ACIA::Clock(int ips) {
    if (!isShifting) [[likely]] {
        if (!echoPending && ((STATUS & ACIA_STATUS_TDRE) != 0 || (CMD & ACIA_CMD_DTR) == 0 || (CMD & ACIA_CMD_ECHO) != 0)) [[likely]] {
            return;
        }
        ClockSlow(ips);
        return;
    }
    if (--shiftRegisterTicks > 0) [[likely]] {
        return;
    }
    ClockSlow(ips);
}

}  // namespace Hardware
