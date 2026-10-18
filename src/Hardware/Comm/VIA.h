#pragma once

#include <functional>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

class VIA : public IBusDevice {
   public:
    VIA();
    void Reset() override;

    // IBusDevice implementation
    Byte Read(Word address) override;
    Byte ReadDirect(Word address) override;
    void Write(Word address, Byte data) override;
    [[nodiscard]] inline std::string GetName() const override;

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& inStream) override;

    [[nodiscard]] Byte GetPortA() const;
    [[nodiscard]] Byte GetPortB() const;

    inline void SetPortBCallback(std::function<void(Byte)> callback);

    [[nodiscard]] Byte GetDDRB() const;
    [[nodiscard]] Byte GetT1L_L() const;

    // External Input Setters (to simulate pins)
    void SetInputA(Byte val);
    void SetInputB(Byte val);
    void SetCA1(bool val);
    void SetCA2(bool val);
    void SetCB1(bool val);
    void SetCB2(bool val);

    inline void Clock();
    inline void Clock(uint32_t cycles);
    [[nodiscard]] inline bool isIRQAsserted() const;
    [[nodiscard]] bool IsIdle() const { return !anyActive; }

   private:
    Byte orb = 0;    // 0x00 Output Register B
    Byte ora = 0;    // 0x01 Output Register A
    Byte ddrb = 0;   // 0x02 Data Direction Register B
    Byte ddra = 0;   // 0x03 Data Direction Register A
    Byte t1l_l = 0;  // 0x06 Timer 1 Latch Low
    Byte t1l_h = 0;  // 0x07 Timer 1 Latch High
    Byte sr = 0;     // 0x0A Shift Register
    Byte acr = 0;    // 0x0B Auxiliary Control Register
    Byte pcr = 0;    // 0x0C Peripheral Control Register
    Byte ifr = 0;    // 0x0D Interrupt Flag Register
    Byte ier = 0;    // 0x0E Interrupt Enable Register

    // Internal state
    Word t1c = 0xFFFF;  // Timer 1 Counter (16-bit internal)
    Word t1l = 0xFFFF;  // Timer 1 Latch (16-bit internal)
    Word t2c = 0xFFFF;  // Timer 2 Counter (16-bit internal)
    Word t2l = 0xFFFF;  // Timer 2 Latch (16-bit internal)

    bool t1_interrupt_armed = false;  // Is T1 interrupt armed?
    bool t2_interrupt_armed = false;  // Is T2 interrupt armed?
    bool t1_pb7_output = true;        // State of PB7 due to specific T1 mode

    Byte ira = 0x00;  // Input Register A (External Pins)
    Byte irb = 0x00;  // Input Register B (External Pins)
    Byte ira_latched = 0;
    Byte irb_latched = 0;
    bool ira_latch_full = false;
    bool irb_latch_full = false;

    // Pulse Counting & CB state
    bool ca1_in = true;
    bool ca2_in = true;
    bool cb1_in = true;
    bool cb2_in = true;

    // Shift Register state
    Byte sr_cnt = 0;         // Number of bits shifted so far (for 8-bit cycle)
    bool sr_active = false;  // Is SR currently shifting?
    bool sr_out_cb2 = true;  // State of CB2 output for SR modes

    bool anyActive = false;
    void UpdateAnyActive() {
        Byte sr_mode = (acr >> 2) & 0x07;
        bool t2_sr_reload = (sr_mode == 1 || sr_mode == 4 || sr_mode == 5);
        anyActive = t1_interrupt_armed || ((acr & 0xC0) != 0) || t2_interrupt_armed || t2_sr_reload ||
                    sr_active || ca2_pulse_pending || cb1_pulse_pending || cb2_pulse_pending;
    }

    void ClockSlow();
    void UpdateIRQ();
    void HandleTimer1();
    bool HandleTimer2();
    void HandleShiftRegister(bool timer2_underflowed);

    // Output Callbacks (triggered when Output Pins change)
    std::function<void(Byte)> port_b_callback;
    std::function<void(bool)> ca2_callback;
    std::function<void(bool)> cb1_callback;
    std::function<void(bool)> cb2_callback;

    bool ca2_out = true;
    bool cb1_out = true;
    bool cb2_out = true;
    bool ca2_pulse_pending = false;
    bool cb1_pulse_pending = false;
    bool cb2_pulse_pending = false;

    inline void UpdateCA2Output(bool val);
    inline void UpdateCB1Output(bool val);
    inline void UpdateCB2Output(bool val);

    [[nodiscard]] inline Byte GetEffectiveORB() const;
};

}  // namespace Hardware

namespace Hardware {

inline std::string VIA::GetName() const { return "VIA"; }

inline void VIA::SetPortBCallback(std::function<void(Byte)> callback) { port_b_callback = std::move(callback); }

inline void VIA::UpdateCA2Output(bool val) {
    if (ca2_out != val) {
        ca2_out = val;
        if (ca2_callback) ca2_callback(ca2_out);
    }
}

inline void VIA::UpdateCB1Output(bool val) {
    if (cb1_out != val) {
        cb1_out = val;
        if (cb1_callback) cb1_callback(cb1_out);
    }
}

inline void VIA::UpdateCB2Output(bool val) {
    if (cb2_out != val) {
        cb2_out = val;
        if (cb2_callback) cb2_callback(cb2_out);
    }
}

inline bool VIA::isIRQAsserted() const { return (ifr & 0x80) != 0; }

inline Byte VIA::GetEffectiveORB() const {
    if ((acr & 0x80) != 0) {
        return (orb & ~0x80) | (t1_pb7_output ? 0x80 : 0);
    }
    return orb;
}

inline void VIA::HandleTimer1() {
    t1c--;
    if (t1c == 0xFFFF) [[unlikely]] {  // Underflow from 0 to -1 (0xFFFF)
        // Interrupt Logic
        if (t1_interrupt_armed) {
            ifr |= 0x40;  // Set T1 interrupt
            if ((acr & 0x40) == 0) {
                // One-shot mode, disable future interrupts until reloaded
                t1_interrupt_armed = false;
            }
        }

        if ((acr & 0x40) != 0) {
            // Continuous interrupts mode
            t1c = t1l;    // Reload
        }

        // PB7 Toggling (ACR bit 7)
        if ((acr & 0x80) != 0) {
            if ((acr & 0x40) == 0) {
                // One-shot: raise PB7 on timeout
                t1_pb7_output = true;
            } else {
                // Continuous: toggle PB7 on timeout
                t1_pb7_output = !t1_pb7_output;
            }
            // If DDRB bit 7 is output, this reflects on the pin
            if ((ddrb & 0x80) != 0) {
                if (port_b_callback) {
                    port_b_callback((irb & ~ddrb) | (GetEffectiveORB() & ddrb));
                }
            }
        }

        UpdateIRQ();
    }
}

inline bool VIA::HandleTimer2() {
    bool underflowed = false;
    // Mode check: ACR bit 5 (0 = One shot)
    // If (acr & 0x20) == 0 -> One Shot counts PHI2
    // If (acr & 0x20) == 1 -> Pulse Counting (handled in SetInputB)
    if ((acr & 0x20) == 0) {
        t2c--;
        if (t2c == 0xFFFF) [[unlikely]] {
            underflowed = true;
            if (t2_interrupt_armed) {
                ifr |= 0x20;
                t2_interrupt_armed = false;  // T2 one-shot interrupt disabled until reloaded
                UpdateIRQ();
            }
            
            // In Shift Register modes 1, 4, 5, Timer 2 acts as a baud rate generator and auto-reloads.
            Byte sr_mode = (acr >> 2) & 0x07;
            if (sr_mode == 1 || sr_mode == 4 || sr_mode == 5) {
                t2c = t2l;
            }
        }
    }
    return underflowed;
}

inline void VIA::HandleShiftRegister(bool timer2_underflowed) {
    if (!sr_active) {
        return;
    }

    // ACR [4:2] determines mode.
    Byte sr_mode = (acr >> 2) & 0x07;
    if (sr_mode == 0) {
        return;
    }

    bool shift = false;

    switch (sr_mode) {
        case 1:  // Shift In (T2)
        case 4:  // Shift Out (Free Run / T2)
        case 5:  // Shift Out (T2)
            if (timer2_underflowed) {
                // Generate a CB1 pulse (half a T2 cycle wide, but we just pulse it here)
                UpdateCB1Output(false); // Drop CB1
                shift = true;
                cb1_pulse_pending = true;
            }
            break;
        case 2:  // Shift In (PHI2)
        case 6:  // Shift Out (PHI2)
            UpdateCB1Output(false);
            shift = true;
            cb1_pulse_pending = true;
            break;
        case 3:  // Shift In (External CB1)
        case 7:  // Shift Out (External CB1)
            // Shift is triggered in SetCB1, handled separately
            break;
        default:
            break;
    }

    if (shift) {
        if ((sr_mode & 0x04) != 0) {  // Shift Out (Modes 4,5,6,7)
            // Shift MSB out to CB2
            sr_out_cb2 = (sr & 0x80) != 0;
            UpdateCB2Output(sr_out_cb2);
            sr = (sr << 1) | (sr_out_cb2 ? 1 : 0);
        } else {  // Shift In (Modes 1,2,3)
            // Shift CB2 into LSB
            bool cb2_val = cb2_in;
            sr = (sr << 1) | (cb2_val ? 1 : 0);
        }

        sr_cnt++;
        if (sr_cnt == 8) {
            ifr |= 0x04;  // Set SR Interrupt
            if (sr_mode != 4) {
                sr_active = false;  // Stop unless Free Run (Mode 4)
            }
            sr_cnt = 0;
            UpdateIRQ();
        }
    }
}

inline void VIA::Clock() {
    if (!anyActive || (!ca2_pulse_pending && !cb1_pulse_pending && !cb2_pulse_pending && !sr_active && t1c > 0 &&
                       ((acr & 0x20) != 0 || t2c > 0))) [[likely]] {
        t1c--;
        if ((acr & 0x20) == 0) {
            t2c--;
        }
        return;
    }
    ClockSlow();
}

inline void VIA::Clock(uint32_t cycles) {
    if (!anyActive) [[likely]] {
        t1c = static_cast<Word>(t1c - cycles);
        if ((acr & 0x20) == 0) {
            t2c = static_cast<Word>(t2c - cycles);
        }
        return;
    }
    for (uint32_t i = 0; i < cycles; ++i) {
        ClockSlow();
    }
}

}  // namespace Hardware
