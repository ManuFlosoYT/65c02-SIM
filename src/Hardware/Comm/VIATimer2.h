#pragma once

#include <cstdint>

namespace Hardware {

using Byte = uint8_t;
using Word = uint16_t;

struct VIATimer2 {
    Word t2c = 0xFFFF;
    Word t2l = 0xFFFF;
    bool t2_interrupt_armed = false;

    inline void Reset() {
        t2c = 0xFFFF;
        t2l = 0xFFFF;
        t2_interrupt_armed = false;
    }

    [[nodiscard]] inline Byte ReadLow() const { return static_cast<Byte>(t2c & 0xFF); }
    [[nodiscard]] inline Byte ReadHigh() const { return static_cast<Byte>(t2c >> 8); }

    inline void WriteLow(Byte data) {
        t2l = (t2l & 0xFF00) | data;
    }

    inline void WriteHigh(Byte data) {
        t2l = (static_cast<Word>(data) << 8) | (t2l & 0xFF);
        t2c = t2l;
        t2_interrupt_armed = true;
    }

    inline void Clock() {
        t2c--;
    }

    struct TickResult {
        bool underflow = false;
        bool interruptTriggered = false;
    };

    inline TickResult Clock(Byte acr) {
        TickResult result;
        if ((acr & 0x20) == 0) {
            t2c--;
            if (t2c == 0xFFFF) [[unlikely]] {
                result.underflow = true;
                if (t2_interrupt_armed) {
                    result.interruptTriggered = true;
                    t2_interrupt_armed = false;
                }

                Byte sr_mode = (acr >> 2) & 0x07;
                if (sr_mode == 1 || sr_mode == 4 || sr_mode == 5) {
                    t2c = t2l;
                }
            }
        }
        return result;
    }

    inline TickResult PulsePB6(Byte acr) {
        TickResult result;
        t2c--;
        if (t2c == 0xFFFF) [[unlikely]] {
            result.underflow = true;
            if (t2_interrupt_armed) {
                result.interruptTriggered = true;
                t2_interrupt_armed = false;
            }

            Byte sr_mode = (acr >> 2) & 0x07;
            if (sr_mode == 1 || sr_mode == 4 || sr_mode == 5) {
                t2c = t2l;
            }
        }
        return result;
    }
};

}  // namespace Hardware
