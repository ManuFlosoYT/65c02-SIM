#pragma once

#include <functional>

#include "Hardware/Core/IBusDevice.h"
#include "Hardware/Comm/VIATimer1.h"
#include "Hardware/Comm/VIATimer2.h"
#include "Hardware/Comm/VIAShiftRegister.h"

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

    [[nodiscard]] const VIATimer1& GetTimer1() const { return timer1; }
    [[nodiscard]] VIATimer1& GetTimer1() { return timer1; }
    [[nodiscard]] const VIATimer2& GetTimer2() const { return timer2; }
    [[nodiscard]] VIATimer2& GetTimer2() { return timer2; }
    [[nodiscard]] const VIAShiftRegister& GetShiftRegister() const { return shiftRegister; }
    [[nodiscard]] VIAShiftRegister& GetShiftRegister() { return shiftRegister; }

    // External Input Setters (to simulate pins)
    void SetInputA(Byte val);
    void SetInputB(Byte val);
    void SetCA1(bool val);
    void SetCA2(bool val);
    void SetCB1(bool val);
    void SetCB2(bool val);

    [[nodiscard]] inline bool GetCA1() const { return ca1_in; }
    [[nodiscard]] inline bool GetCA2() const { return (((pcr >> 1) & 0x07) >= 4) ? ca2_out : ca2_in; }
    [[nodiscard]] inline bool GetCB1() const { Byte m = (acr >> 2) & 0x07; return (m == 1 || m == 2 || m == 4 || m == 5 || m == 6) ? cb1_out : cb1_in; }
    [[nodiscard]] inline bool GetCB2() const { Byte m = (acr >> 2) & 0x07; return (m != 0) ? shiftRegister.sr_out_cb2 : (((pcr >> 5) & 0x07) >= 4 ? cb2_out : cb2_in); }

    inline void Clock();
    inline void Clock(uint32_t cycles);
    [[nodiscard]] inline bool isIRQAsserted() const;
    [[nodiscard]] bool IsIdle() const { return !anyActive; }

   private:
    Byte orb = 0;    // 0x00 Output Register B
    Byte ora = 0;    // 0x01 Output Register A
    Byte ddrb = 0;   // 0x02 Data Direction Register B
    Byte ddra = 0;   // 0x03 Data Direction Register A
    Byte acr = 0;    // 0x0B Auxiliary Control Register
    Byte pcr = 0;    // 0x0C Peripheral Control Register
    Byte ifr = 0;    // 0x0D Interrupt Flag Register
    Byte ier = 0;    // 0x0E Interrupt Enable Register

    VIATimer1 timer1;
    VIATimer2 timer2;
    VIAShiftRegister shiftRegister;

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

    bool anyActive = false;
    void UpdateAnyActive() {
        Byte sr_mode = (acr >> 2) & 0x07;
        bool t2_sr_reload = (sr_mode == 1 || sr_mode == 4 || sr_mode == 5);
        anyActive = timer1.t1Active || ((acr & 0xC0) != 0) || timer2.t2_interrupt_armed || t2_sr_reload ||
                    shiftRegister.sr_active || ca2_pulse_pending || cb1_pulse_pending || cb2_pulse_pending;
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
        return (orb & ~0x80) | (timer1.t1_pb7_output ? 0x80 : 0);
    }
    return orb;
}

inline void VIA::HandleTimer1() {
    auto res = timer1.Clock(acr);
    if (res.underflow) [[unlikely]] {
        if (res.interruptTriggered) {
            ifr |= 0x40;
        }

        if (res.pb7Toggled && ((ddrb & 0x80) != 0)) {
            if (port_b_callback) {
                port_b_callback((irb & ~ddrb) | (GetEffectiveORB() & ddrb));
            }
        }

        UpdateIRQ();
    }
}

inline bool VIA::HandleTimer2() {
    auto res = timer2.Clock(acr);
    if (res.interruptTriggered) {
        ifr |= 0x20;
        UpdateIRQ();
    }
    return res.underflow;
}

inline void VIA::HandleShiftRegister(bool timer2_underflowed) {
    if (!shiftRegister.sr_active) {
        return;
    }

    auto res = shiftRegister.Clock(acr, timer2_underflowed, cb2_in);
    if (res.cb1PulsePending) {
        UpdateCB1Output(false);
        cb1_pulse_pending = true;
    }
    if (res.cb2Updated) {
        UpdateCB2Output(res.cb2Out);
    }
    if (res.interruptTriggered) {
        ifr |= 0x04;
        UpdateIRQ();
    }
}

inline void VIA::Clock() {
    if (!anyActive || (!ca2_pulse_pending && !cb1_pulse_pending && !cb2_pulse_pending && !shiftRegister.sr_active && timer1.t1c > 0 &&
                       ((acr & 0x20) != 0 || timer2.t2c > 0))) [[likely]] {
        timer1.t1c--;
        if ((acr & 0x20) == 0) {
            timer2.t2c--;
        }
        return;
    }
    ClockSlow();
}

inline void VIA::Clock(uint32_t cycles) {
    if (!anyActive || (!ca2_pulse_pending && !cb1_pulse_pending && !cb2_pulse_pending && !shiftRegister.sr_active &&
                       timer1.t1c >= cycles && ((acr & 0x20) != 0 || timer2.t2c >= cycles))) [[likely]] {
        timer1.t1c = static_cast<Word>(timer1.t1c - cycles);
        if ((acr & 0x20) == 0) {
            timer2.t2c = static_cast<Word>(timer2.t2c - cycles);
        }
        return;
    }
    for (uint32_t i = 0; i < cycles; ++i) {
        ClockSlow();
    }
}

}  // namespace Hardware
