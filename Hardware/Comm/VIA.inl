// VIA.inl — Inline implementations for Hardware::VIA

namespace Hardware {

inline std::string VIA::GetName() const { return "VIA"; }

inline void VIA::SetPortBCallback(std::function<void(Byte)> cb) {
    port_b_callback = cb;
}

inline bool VIA::isIRQAsserted() const { return (ifr & 0x80) != 0; }

inline void VIA::Clock() {
    if (!anyActive) return;

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

}  // namespace Hardware
