#pragma once

#include <cstdint>

namespace Hardware {

using Byte = uint8_t;

struct LatchResult {
    bool ready = false;
    bool is_data = false;
    Byte value = 0;

    constexpr explicit operator bool() const noexcept { return ready; }
};

struct HD44780BusLatch {
    Byte PORTB_DATA = 0;
    bool four_bit_mode = false;
    bool waiting_low_nibble = false;
    Byte current_high_nibble = 0;
    Byte last_portb = 0;
    bool is_init = false;

    constexpr void Reset() noexcept {
        PORTB_DATA = 0;
        four_bit_mode = false;
        waiting_low_nibble = false;
        current_high_nibble = 0;
        last_portb = 0;
        is_init = false;
    }

    constexpr LatchResult ProcessPortB(Byte val) noexcept {
        Byte old_val = last_portb;
        last_portb = val;
        PORTB_DATA = val;

        bool old_E = (old_val & 0x40) != 0;
        bool new_E = (val & 0x40) != 0;

        if (old_E && !new_E) {
            Byte data_nibble = val & 0x0F;
            bool regSelect = (val & 0x10) != 0;
            bool readWrite = (val & 0x20) != 0;

            if (readWrite) {
                return {};
            }

            if (!four_bit_mode) {
                if (!regSelect && data_nibble == 0x02) {
                    four_bit_mode = true;
                    waiting_low_nibble = false;
                    is_init = true;
                }
            } else {
                if (!waiting_low_nibble) {
                    current_high_nibble = data_nibble;
                    waiting_low_nibble = true;
                } else {
                    Byte low_nibble = data_nibble;
                    Byte full_byte = static_cast<Byte>((current_high_nibble << 4) | low_nibble);
                    waiting_low_nibble = false;

                    return LatchResult{
                        .ready = true,
                        .is_data = regSelect,
                        .value = full_byte
                    };
                }
            }
        }

        return {};
    }
};

}  // namespace Hardware
