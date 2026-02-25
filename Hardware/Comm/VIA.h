#pragma once

#include <functional>
#include <iostream>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

class VIA : public IBusDevice {
public:
    VIA();
    void Reset() override;

    // IBusDevice implementation
    Byte Read(Word address) override;
    void Write(Word address, Byte data) override;
    inline std::string GetName() const override;

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& in) override;

    Byte GetPortA() const;
    Byte GetPortB() const;
    void SetPortA(Byte val);
    void SetPortB(Byte val);

    inline void SetPortBCallback(std::function<void(Byte)> cb);

    Byte GetDDRB() const;
    void SetDDRB(Byte val);

    Byte GetDDRA() const;
    void SetDDRA(Byte val);

    Byte GetT1C_L() const;
    void SetT1C_L(Byte val);

    Byte GetT1C_H() const;
    void SetT1C_H(Byte val);

    Byte GetT1L_L() const;
    void SetT1L_L(Byte val);

    Byte GetT1L_H() const;
    void SetT1L_H(Byte val);

    Byte GetT2C_L() const;
    void SetT2C_L(Byte val);

    Byte GetT2C_H() const;
    void SetT2C_H(Byte val);

    Byte GetSR() const;
    void SetSR(Byte val);

    Byte GetACR() const;
    void SetACR(Byte val);

    Byte GetPCR() const;
    void SetPCR(Byte val);

    Byte GetIFR() const;
    void SetIFR(Byte val);

    Byte GetIER() const;
    void SetIER(Byte val);

    Byte GetORA_NH() const;
    void SetORA_NH(Byte val);

    // External Input Setters (to simulate pins)
    void SetInputA(Byte val);
    void SetInputB(Byte val);
    void SetCA1(bool val);
    void SetCA2(bool val);
    void SetCB1(bool val);
    void SetCB2(bool val);

    inline void Clock();
    inline bool isIRQAsserted() const;

private:
    Byte orb;     // 0x00 Output Register B
    Byte ora;     // 0x01 Output Register A
    Byte ddrb;    // 0x02 Data Direction Register B
    Byte ddra;    // 0x03 Data Direction Register A
    Byte t1c_l;   // 0x04 Timer 1 Counter Low
    Byte t1c_h;   // 0x05 Timer 1 Counter High
    Byte t1l_l;   // 0x06 Timer 1 Latch Low
    Byte t1l_h;   // 0x07 Timer 1 Latch High
    Byte t2c_l;   // 0x08 Timer 2 Counter Low
    Byte t2c_h;   // 0x09 Timer 2 Counter High
    Byte sr;      // 0x0A Shift Register
    Byte acr;     // 0x0B Auxiliary Control Register
    Byte pcr;     // 0x0C Peripheral Control Register
    Byte ifr;     // 0x0D Interrupt Flag Register
    Byte ier;     // 0x0E Interrupt Enable Register
    Byte ora_nh;  // 0x0F Output Register A (No Handshake)

    // Internal state
    Word t1c;  // Timer 1 Counter (16-bit internal)
    Word t1l;  // Timer 1 Latch (16-bit internal)
    Word t2c;  // Timer 2 Counter (16-bit internal)
    Word t2l;  // Timer 2 Latch (16-bit internal)

    bool t1_active;      // Is Timer 1 currently counting?
    bool t2_active;      // Is Timer 2 currently counting?
    bool t1_pb7_output;  // State of PB7 due to specific T1 mode

    Byte ira;  // Input Register A (External Pins)
    Byte irb;  // Input Register B (External Pins)

    // Pulse Counting & CB state
    Byte last_irb;
    bool ca1_in;
    bool ca2_in;
    bool cb1_in;
    bool cb2_in;
    bool last_ca1;
    bool last_ca2;
    bool last_cb1;
    bool last_cb2;

    // Shift Register state
    Byte sr_cnt;      // Number of bits shifted so far (for 8-bit cycle)
    bool sr_active;   // Is SR currently shifting?
    bool sr_out_cb2;  // State of CB2 output for SR modes

    bool anyActive = false;
    inline void UpdateAnyActive() {
        anyActive = t1_active || t2_active || sr_active;
    }

    void UpdateIRQ();

    // Output Callbacks (triggered when Output Pins change)
    std::function<void(Byte)> port_a_callback;
    std::function<void(Byte)> port_b_callback;
};

}  // namespace Hardware

#include "Hardware/Comm/VIA.inl"
