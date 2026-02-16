#include "VIA.h"

namespace Hardware {

VIA::VIA() { Reset(); }

void VIA::Reset() {
    orb = 0;
    ora = 0;
    ddrb = 0;
    ddra = 0;
    t1c_l = 0;
    t1c_h = 0;
    t1l_l = 0;
    t1l_h = 0;
    t2c_l = 0;
    t2c_h = 0;
    sr = 0;
    acr = 0;
    pcr = 0;
    ifr = 0;
    ier = 0;
    ora_nh = 0;

    t1c = 0xFFFF;
    t1l = 0xFFFF;
    t2c = 0xFFFF;
    t2l = 0xFFFF;
    t1_active = false;
    t2_active = false;

    ira = 0x00;
    irb = 0x00;
    last_irb = 0x00;
    t1_pb7_output = true;
    cb1_in = true;
    cb2_in = true;
    sr_cnt = 0;
    sr_active = false;
    sr_out_cb2 = true;
}

void VIA::Init(Mem& mem) {
    // 0x6000 - ORB / IRB (PORTB)
    mem.SetWriteHook(PORTB, [this](Word dir, Byte val) {
        Byte old_pins = (irb & ~ddrb) | (orb & ddrb);
        this->orb = val;
        // Check if output pins changed
        Byte new_pins = (irb & ~ddrb) | (orb & ddrb);
        if (port_b_callback && (old_pins != new_pins)) {
            port_b_callback(new_pins);
        }
    });
    mem.SetReadHook(PORTB, [this](Word dir) {
        // Read Input Register B
        // Pins are (Input & ~DDRB) | (ORB & DDRB)
        // If DDRB bit is 0 (Input), we read the Pin (irb).
        // If DDRB bit is 1 (Output), we read the Output Register (orb).
        // Actually, 6522 spec says reading PORTB reads the actual pin state for
        // input pins, and the output register for output pins. Pin state =
        // (InputVal & ~DDR) | (OutputVal & DDR).
        return (irb & ~ddrb) | (orb & ddrb);
    });

    // 0x6001 - ORA / IRA (PORTA)
    mem.SetWriteHook(PORTA, [this](Word dir, Byte val) {
        Byte old_pins = (ira & ~ddra) | (ora & ddra);
        this->ora = val;
        Byte new_pins = (ira & ~ddra) | (ora & ddra);
        if (port_a_callback && (old_pins != new_pins)) {
            port_a_callback(new_pins);
        }
    });
    mem.SetReadHook(PORTA, [this](Word dir) {
        // Read Input Register A
        // Similar logic to PORTB, but ORA read clears interrupt flags usually?
        // Reading ORA clears CA1/CA2/CB1/CB2 interrupt flags in some modes.
        // For now just port mix.
        return (ira & ~ddra) | (ora & ddra);
    });

    // 0x6002 - DDRB
    mem.SetWriteHook(DDRB, [this](Word dir, Byte val) {
        Byte old_pins = (irb & ~ddrb) | (orb & ddrb);
        this->ddrb = val;
        Byte new_pins = (irb & ~ddrb) | (orb & ddrb);
        if (port_b_callback && (old_pins != new_pins)) {
            port_b_callback(new_pins);
        }
    });
    mem.SetReadHook(DDRB, [this](Word dir) { return this->ddrb; });

    // 0x6003 - DDRA
    mem.SetWriteHook(DDRA, [this](Word dir, Byte val) {
        Byte old_pins = (ira & ~ddra) | (ora & ddra);
        this->ddra = val;
        Byte new_pins = (ira & ~ddra) | (ora & ddra);
        if (port_a_callback && (old_pins != new_pins)) {
            port_a_callback(new_pins);
        }
    });
    mem.SetReadHook(DDRA, [this](Word dir) { return this->ddra; });

    // 0x6004 - T1C_L (Write: Latch Low, Read: Counter Low & Clear Interrupt)
    mem.SetWriteHook(T1C_L, [this](Word dir, Byte val) { this->t1l_l = val; });
    mem.SetReadHook(T1C_L, [this](Word dir) {
        this->ifr &= ~0x40;  // Clear T1 interrupt flag on read
        UpdateIRQ();
        return (Byte)(this->t1c & 0xFF);
    });

    // 0x6005 - T1C_H (Write: Latch High & Transfer, Read: Counter High)
    mem.SetWriteHook(T1C_H, [this](Word dir, Byte val) {
        this->t1c_h = val;
        this->t1l_h = val;
        this->t1l = (val << 8) | this->t1l_l;
        this->t1c = this->t1l;  // Load counter with latch
        this->ifr &= ~0x40;     // Clear T1 interrupt flag
        this->t1_active = true;
        this->t1_pb7_output = false;  // T1 output goes low on load (if enabled)

        // If PB7 output enabled, update pin
        if ((acr & 0x80) && (ddrb & 0x80)) {
            orb &= ~0x80;  // Low
            if (port_b_callback) port_b_callback((irb & ~ddrb) | (orb & ddrb));
        }

        UpdateIRQ();
    });
    mem.SetReadHook(T1C_H, [this](Word dir) { return (Byte)(this->t1c >> 8); });

    // 0x6006 - T1L_L (Write: Latch Low, Read: Latch Low)
    mem.SetWriteHook(T1L_L, [this](Word dir, Byte val) {
        this->t1l_l = val;
        this->t1l = (this->t1l & 0xFF00) | val;
    });
    mem.SetReadHook(T1L_L, [this](Word dir) { return this->t1l_l; });

    // 0x6007 - T1L_H (Write: Latch High, Read: Latch High)
    mem.SetWriteHook(T1L_H, [this](Word dir, Byte val) {
        this->t1l_h = val;
        this->t1l = (val << 8) | this->t1l_l;
        this->ifr &= ~0x40;  // Clear T1 interrupt flag (some docs say this,
                             // others don't for latch write)
        UpdateIRQ();
    });
    mem.SetReadHook(T1L_H, [this](Word dir) { return this->t1l_h; });

    // 0x6008 - T2C_L (Write: Latch Low, Read: Counter Low)
    mem.SetWriteHook(T2C_L, [this](Word dir, Byte val) {
        this->t2l = (this->t2l & 0xFF00) | val;
        this->t2c_l = val;
    });
    mem.SetReadHook(T2C_L, [this](Word dir) {
        this->ifr &= ~0x20;  // Clear T2 interrupt flag on read
        UpdateIRQ();
        return (Byte)(this->t2c & 0xFF);
    });

    // 0x6009 - T2C_H (Write: Counter High & Start, Read: Counter High)
    mem.SetWriteHook(T2C_H, [this](Word dir, Byte val) {
        this->t2c_h = val;
        this->t2l = (val << 8) | (this->t2l & 0xFF);
        this->t2c = this->t2l;
        this->ifr &= ~0x20;  // Clear T2 interrupt flag
        this->t2_active = true;
        UpdateIRQ();
    });
    mem.SetReadHook(T2C_H, [this](Word dir) { return (Byte)(this->t2c >> 8); });

    // 0x600A - SR
    mem.SetWriteHook(SR, [this](Word dir, Byte val) {
        this->sr = val;
        this->ifr &= ~0x04;      // Clear SR interrupt bit
        this->sr_active = true;  // Start shifting (if mode enables it)
        this->sr_cnt = 0;        // Reset bit counter
        UpdateIRQ();
    });
    mem.SetReadHook(SR, [this](Word dir) {
        this->ifr &= ~0x04;  // Clear SR interrupt bit on read
        this->sr_active =
            true;  // Start shifting (if mode enables it, e.g. shifting in)
        this->sr_cnt = 0;
        UpdateIRQ();
        return this->sr;
    });

    // 0x600B - ACR
    mem.SetWriteHook(ACR, [this](Word dir, Byte val) { this->acr = val; });
    mem.SetReadHook(ACR, [this](Word dir) { return this->acr; });

    // 0x600C - PCR
    mem.SetWriteHook(PCR, [this](Word dir, Byte val) { this->pcr = val; });
    mem.SetReadHook(PCR, [this](Word dir) { return this->pcr; });

    // 0x600D - IFR
    // Typically IFR bits are cleared by writing 1 to them
    mem.SetWriteHook(IFR, [this](Word dir, Byte val) {
        // Only bits written as 1 are cleared. Bit 7 is top-level IRQ flag,
        // read-only.
        this->ifr &= ~(val & 0x7F);
        UpdateIRQ();
    });
    mem.SetReadHook(IFR, [this](Word dir) { return this->ifr; });

    // 0x600E - IER
    mem.SetWriteHook(IER, [this](Word dir, Byte val) {
        // Bit 7 determines set (1) or clear (0)
        if (val & 0x80) {
            this->ier |= (val & 0x7F);
        } else {
            this->ier &= ~(val & 0x7F);
        }
        this->ier &= 0x7F;  // Keep bit 7 always 0
        UpdateIRQ();
    });
    // Bit 7 always 1 on read
    mem.SetReadHook(IER, [this](Word dir) { return this->ier | 0x80; });

    // 0x600F - ORA_NH (and ORA macro in Mem.h points to 0x600F)
    mem.SetWriteHook(ORA_NH, [this](Word dir, Byte val) {
        this->ora_nh = val;
        this->ora = val;  // Also update ORA
    });
    mem.SetReadHook(ORA_NH, [this](Word dir) { return this->ora_nh; });
}

void VIA::Clock() {
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
                // t1_active = false; // Typically continues counting down for
                // phase checks but interrupt fires once? W65C22 specs: T1
                // continues counting to 0, -1, -2... interrupts generated on
                // 0->-1 transition.
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
            case 6:  // Shift Out (PHI2)
            {
                shift = true;
                break;
            }
            case 4:  // Shift Out (Free Run / T2)
                // Assuming T2 free run logic or just simple PHI2 for now if T2
                // not strictly modeled for this Spec says Free Run uses T2 low
                // order byte for rate. Simplified: Shift every cycle for this
                // task or check T2. Let's implement basics: Shift every call
                // for PHI2 modes.
                shift = true;
                break;
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

void VIA::UpdateIRQ() {
    if ((ifr & ier) & 0x7F) {
        ifr |= 0x80;  // Set IRQ bit if any enabled interrupt is active
    } else {
        ifr &= 0x7F;  // Clear IRQ bit
    }
}

bool VIA::isIRQAsserted() const { return (ifr & 0x80) != 0; }

Byte VIA::GetPortA() const { return (ira & ~ddra) | (ora & ddra); }
Byte VIA::GetPortB() const { return (irb & ~ddrb) | (orb & ddrb); }

void VIA::SetInputA(Byte val) {
    ira = val;
    // Logic: if CA1/CA2 transition, IRQ. (Not fully impl yet)
}

void VIA::SetInputB(Byte val) {
    // Pulse Counting for T2 (PB6)
    // Check PB6 transition high-to-low
    if ((acr & 0x20) && t2_active) {
        // PB6 is bit 6
        bool old_pb6 = (irb & 0x40) != 0;
        bool new_pb6 = (val & 0x40) != 0;

        if (old_pb6 && !new_pb6) {  // Falling edge
            t2c--;
            if (t2c == 0xFFFF) {
                ifr |= 0x20;
                t2_active = false;  // T2 stops for pulse counting? Spec says it
                                    // continues?
                // Spec: T2 Pulse Counting counts down 0 -> -1 then Interrupt.
                // Usually behaves like free runner if reloaded?
                // For valid implementation: One interrupt, then it wraps.
                UpdateIRQ();
            }
        }
    }

    irb = val;
}

void VIA::SetCB1(bool val) { cb1_in = val; }
void VIA::SetCB2(bool val) { cb2_in = val; }

void VIA::SetPortA(Byte val) { ora = val; }
void VIA::SetPortB(Byte val) { orb = val; }

Byte VIA::GetDDRB() const { return ddrb; }
void VIA::SetDDRB(Byte val) { ddrb = val; }

Byte VIA::GetDDRA() const { return ddra; }
void VIA::SetDDRA(Byte val) { ddra = val; }

Byte VIA::GetT1C_L() const { return t1c_l; }
void VIA::SetT1C_L(Byte val) { t1c_l = val; }

Byte VIA::GetT1C_H() const { return t1c_h; }
void VIA::SetT1C_H(Byte val) { t1c_h = val; }

Byte VIA::GetT1L_L() const { return t1l_l; }
void VIA::SetT1L_L(Byte val) { t1l_l = val; }

Byte VIA::GetT1L_H() const { return t1l_h; }
void VIA::SetT1L_H(Byte val) { t1l_h = val; }

Byte VIA::GetT2C_L() const { return t2c_l; }
void VIA::SetT2C_L(Byte val) { t2c_l = val; }

Byte VIA::GetT2C_H() const { return t2c_h; }
void VIA::SetT2C_H(Byte val) { t2c_h = val; }

Byte VIA::GetSR() const { return sr; }
void VIA::SetSR(Byte val) { sr = val; }

Byte VIA::GetACR() const { return acr; }
void VIA::SetACR(Byte val) { acr = val; }

Byte VIA::GetPCR() const { return pcr; }
void VIA::SetPCR(Byte val) { pcr = val; }

Byte VIA::GetIFR() const { return ifr; }
void VIA::SetIFR(Byte val) { ifr = val; }

Byte VIA::GetIER() const { return ier; }
void VIA::SetIER(Byte val) { ier = val; }

Byte VIA::GetORA_NH() const { return ora_nh; }
void VIA::SetORA_NH(Byte val) { ora_nh = val; }

}  // namespace Hardware
