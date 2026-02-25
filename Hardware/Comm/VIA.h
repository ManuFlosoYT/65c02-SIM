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
    std::string GetName() const override { return "VIA"; }

    bool SaveState(std::ostream& out) const;
    bool LoadState(std::istream& in);

    Byte GetPortA() const;
    Byte GetPortB() const;
    void SetPortA(Byte val);
    void SetPortB(Byte val);

    void SetPortBCallback(std::function<void(Byte)> cb) {
        port_b_callback = cb;
    }

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
    void SetCB1(bool val);
    void SetCB2(bool val);

    inline void Clock() {
        // Timer 1
        if (t1_active) {
            t1c--;
            if (t1c == 0xFFFF) {  // Underflow from 0 to -1 (0xFFFF)
                // Interrupt Logic
                if (acr & 0x40) {
                    // Continuous interrupts mode
                    t1c = t1l;    // Reload
                    ifr |= 0x40;  // Set T1 interrupt
                } else {
                    // One-shot mode
                    ifr |= 0x40;  // Set T1 interrupt
                    // t1_active = false; // Typically continues counting down
                    // for phase checks but interrupt fires once? W65C22 specs:
                    // T1 continues counting to 0, -1, -2... interrupts
                    // generated on 0->-1 transition.
                }

                // PB7 Toggling (ACR bit 7)
                if (acr & 0x80) {
                    t1_pb7_output = !t1_pb7_output;
                    // If DDRB bit 7 is output, this reflects on the pin
                    if (ddrb & 0x80) {
                        if (t1_pb7_output)
                            orb |= 0x80;
                        else
                            orb &= ~0x80;
                        if (port_b_callback)
                            port_b_callback((irb & ~ddrb) | (orb & ddrb));
                    }
                }

                UpdateIRQ();
            }
        }

        // Timer 2
        if (t2_active) {
            // Mode check: ACR bit 5 (0 = One shot)
            // If (acr & 0x20) == 0 -> One Shot counts PHI2
            // If (acr & 0x20) == 1 -> Pulse Counting (handled in SetInputB)
            if (!(acr & 0x20)) {
                t2c--;
                if (t2c == 0xFFFF) {
                    ifr |= 0x20;
                    t2_active = false;  // T2 one-shot stops
                    UpdateIRQ();
                }
            }
        }

        // Shift Register
        // ACR [4:2] determines mode.
        // 000: Disabled
        // 001: Shift In (T2)
        // 010: Shift In (PHI2)
        // 011: Shift In (Ext CB1)
        // 100: Shift Out (Free Run / T2)
        // 101: Shift Out (T2)
        // 110: Shift Out (PHI2)
        // 111: Shift Out (Ext CB1)

        Byte sr_mode = (acr >> 2) & 0x07;

        if (sr_active && sr_mode != 0) {
            bool shift = false;

            switch (sr_mode) {
                case 2:  // Shift In (PHI2)
                case 4:  // Shift Out (Free Run / T2)
                case 6:  // Shift Out (PHI2)
                {
                    shift = true;
                    break;
                }
                default:
                    break;
            }

            if (shift) {
                if (sr_mode & 0x04) {  // Shift Out (Modes 4,5,6,7)
                    // Shift MSB out to CB2
                    sr_out_cb2 = (sr & 0x80) != 0;
                    sr = (sr << 1) | 1;
                } else {  // Shift In (Modes 1,2,3)
                    // Shift CB2 into LSB
                    bool cb2_val = cb2_in;
                    sr = (sr << 1) | (cb2_val ? 1 : 0);
                }

                sr_cnt++;
                if (sr_cnt == 8) {
                    ifr |= 0x04;  // Set SR Interrupt
                    if (sr_mode != 4)
                        sr_active = false;  // Stop unless Free Run (Mode 4)
                    sr_cnt = 0;
                    UpdateIRQ();
                }
            }
        }
    }
    bool isIRQAsserted() const;

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
    bool cb1_in;
    bool cb2_in;

    // Shift Register state
    Byte sr_cnt;      // Number of bits shifted so far (for 8-bit cycle)
    bool sr_active;   // Is SR currently shifting?
    bool sr_out_cb2;  // State of CB2 output for SR modes

    void UpdateIRQ();

    // Output Callbacks (triggered when Output Pins change)
    std::function<void(Byte)> port_a_callback;
    std::function<void(Byte)> port_b_callback;
};

}  // namespace Hardware
