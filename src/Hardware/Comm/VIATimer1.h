#pragma once

#include <cstdint>

namespace Hardware {

using Byte = uint8_t;
using Word = uint16_t;

struct VIATimer1 {
    Word t1c = 0xFFFF;
    Word t1l = 0xFFFF;
    Byte t1l_l = 0;
    Byte t1l_h = 0;
    bool t1Active = false;
    bool t1_pb7_output = true;

    inline void Reset() {
        t1c = 0xFFFF;
        t1l = 0xFFFF;
        t1l_l = 0;
        t1l_h = 0;
        t1Active = false;
        t1_pb7_output = true;
    }

    [[nodiscard]] inline Byte ReadLow() const { return static_cast<Byte>(t1c & 0xFF); }
    [[nodiscard]] inline Byte ReadHigh() const { return static_cast<Byte>(t1c >> 8); }
    [[nodiscard]] inline Byte ReadLatchLow() const { return t1l_l; }
    [[nodiscard]] inline Byte ReadLatchHigh() const { return t1l_h; }

    inline void WriteLow(Byte data) {
        t1l_l = data;
        t1l = (t1l & 0xFF00) | data;
    }

    inline void WriteHigh(Byte data) {
        t1l_h = data;
        t1l = (static_cast<Word>(data) << 8) | t1l_l;
        t1c = t1l;
        t1Active = true;
        t1_pb7_output = false;
    }

    inline void WriteCounterLow(Byte data) {
        WriteLow(data);
    }

    inline void WriteCounterHigh(Byte data) {
        WriteHigh(data);
    }

    inline void WriteLatchLow(Byte data) {
        WriteLow(data);
    }

    inline void WriteLatchHigh(Byte data) {
        t1l_h = data;
        t1l = (static_cast<Word>(data) << 8) | t1l_l;
    }

    inline void Clock() {
        t1c--;
    }

    struct TickResult {
        bool underflow = false;
        bool interruptTriggered = false;
        bool pb7Toggled = false;
    };

    inline TickResult Clock(Byte acr) {
        t1c--;
        if (t1c == 0xFFFF) [[unlikely]] {
            t1c = t1l;
            bool irq = false;
            if (t1Active) {
                irq = true;
                if ((acr & 0x40) == 0) {
                    t1Active = false;
                }
            }

            bool pb7Changed = false;
            if ((acr & 0x80) != 0) {
                if ((acr & 0x40) == 0) {
                    t1_pb7_output = true;
                } else {
                    t1_pb7_output = !t1_pb7_output;
                }
                pb7Changed = true;
            }
            return {true, irq, pb7Changed};
        }
        return {false, false, false};
    }
};

}  // namespace Hardware
