#pragma once

#include <cstdint>

namespace Hardware {

using Byte = uint8_t;

struct VIAShiftRegister {
    Byte sr = 0;
    Byte sr_cnt = 0;
    bool sr_active = false;
    bool sr_out_cb2 = true;

    inline void Reset() {
        sr = 0;
        sr_cnt = 0;
        sr_active = false;
        sr_out_cb2 = true;
    }

    inline Byte Read() {
        sr_active = true;
        sr_cnt = 0;
        return sr;
    }

    inline void Write(Byte data) {
        sr = data;
        sr_active = true;
        sr_cnt = 0;
    }

    inline void Clock() {
        // Simple tick
    }

    struct ShiftResult {
        bool cb1PulsePending = false;
        bool cb2Updated = false;
        bool cb2Out = true;
        bool interruptTriggered = false;
    };

    inline ShiftResult Clock(Byte acr, bool timer2_underflowed, bool cb2_in) {
        ShiftResult res;
        if (!sr_active) {
            return res;
        }

        Byte sr_mode = (acr >> 2) & 0x07;
        if (sr_mode == 0) {
            return res;
        }

        bool shift = false;
        switch (sr_mode) {
            case 1:  // Shift In (T2)
            case 4:  // Shift Out (Free Run / T2)
            case 5:  // Shift Out (T2)
                if (timer2_underflowed) {
                    res.cb1PulsePending = true;
                    shift = true;
                }
                break;
            case 2:  // Shift In (PHI2)
            case 6:  // Shift Out (PHI2)
                res.cb1PulsePending = true;
                shift = true;
                break;
            case 3:  // Shift In (External CB1)
            case 7:  // Shift Out (External CB1)
                break;
            default:
                break;
        }

        if (shift) {
            if ((sr_mode & 0x04) != 0) {  // Shift Out (Modes 4, 5, 6, 7)
                sr_out_cb2 = (sr & 0x80) != 0;
                res.cb2Updated = true;
                res.cb2Out = sr_out_cb2;
                sr = static_cast<Byte>((sr << 1) | (sr_out_cb2 ? 1 : 0));
            } else {  // Shift In (Modes 1, 2, 3)
                sr = static_cast<Byte>((sr << 1) | (cb2_in ? 1 : 0));
            }

            sr_cnt++;
            if (sr_cnt == 8) {
                res.interruptTriggered = true;
                if (sr_mode != 4) {
                    sr_active = false;
                }
                sr_cnt = 0;
            }
        }
        return res;
    }

    inline ShiftResult ShiftExternalCB1(Byte acr, bool cb2_in) {
        ShiftResult res;
        if (!sr_active) {
            return res;
        }

        Byte sr_mode = (acr >> 2) & 0x07;
        if (sr_mode == 3 || sr_mode == 7) {
            if (sr_mode == 7) {
                sr_out_cb2 = (sr & 0x80) != 0;
                res.cb2Updated = true;
                res.cb2Out = sr_out_cb2;
                sr = static_cast<Byte>((sr << 1) | (sr_out_cb2 ? 1 : 0));
            } else {
                sr = static_cast<Byte>((sr << 1) | (cb2_in ? 1 : 0));
            }

            sr_cnt++;
            if (sr_cnt == 8) {
                res.interruptTriggered = true;
                sr_active = false;
                sr_cnt = 0;
            }
        }
        return res;
    }
};

}  // namespace Hardware
