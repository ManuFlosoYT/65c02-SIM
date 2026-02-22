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
    mem.SetWriteHook(
        PORTB,
        [](void* context, Word dir, Byte val) {
            auto self = static_cast<VIA*>(context);
            Byte old_pins =
                (self->irb & ~self->ddrb) | (self->orb & self->ddrb);
            self->orb = val;
            Byte new_pins =
                (self->irb & ~self->ddrb) | (self->orb & self->ddrb);
            if (self->port_b_callback && (old_pins != new_pins)) {
                self->port_b_callback(new_pins);
            }
        },
        this);

    mem.SetReadHook(
        PORTB,
        [](void* context, Word dir) -> Byte {
            auto self = static_cast<VIA*>(context);
            return (self->irb & ~self->ddrb) | (self->orb & self->ddrb);
        },
        this);

    mem.SetWriteHook(
        PORTA,
        [](void* context, Word dir, Byte val) {
            auto self = static_cast<VIA*>(context);
            Byte old_pins =
                (self->ira & ~self->ddra) | (self->ora & self->ddra);
            self->ora = val;
            Byte new_pins =
                (self->ira & ~self->ddra) | (self->ora & self->ddra);
            if (self->port_a_callback && (old_pins != new_pins)) {
                self->port_a_callback(new_pins);
            }
        },
        this);

    mem.SetReadHook(
        PORTA,
        [](void* context, Word dir) -> Byte {
            auto self = static_cast<VIA*>(context);
            return (self->ira & ~self->ddra) | (self->ora & self->ddra);
        },
        this);

    mem.SetWriteHook(
        DDRB,
        [](void* context, Word dir, Byte val) {
            auto self = static_cast<VIA*>(context);
            Byte old_pins =
                (self->irb & ~self->ddrb) | (self->orb & self->ddrb);
            self->ddrb = val;
            Byte new_pins =
                (self->irb & ~self->ddrb) | (self->orb & self->ddrb);
            if (self->port_b_callback && (old_pins != new_pins)) {
                self->port_b_callback(new_pins);
            }
        },
        this);
    mem.SetReadHook(
        DDRB,
        [](void* context, Word dir) {
            return static_cast<VIA*>(context)->ddrb;
        },
        this);

    mem.SetWriteHook(
        DDRA,
        [](void* context, Word dir, Byte val) {
            auto self = static_cast<VIA*>(context);
            Byte old_pins =
                (self->ira & ~self->ddra) | (self->ora & self->ddra);
            self->ddra = val;
            Byte new_pins =
                (self->ira & ~self->ddra) | (self->ora & self->ddra);
            if (self->port_a_callback && (old_pins != new_pins)) {
                self->port_a_callback(new_pins);
            }
        },
        this);
    mem.SetReadHook(
        DDRA,
        [](void* context, Word dir) {
            return static_cast<VIA*>(context)->ddra;
        },
        this);

    mem.SetWriteHook(
        T1C_L,
        [](void* context, Word dir, Byte val) {
            static_cast<VIA*>(context)->t1l_l = val;
        },
        this);
    mem.SetReadHook(
        T1C_L,
        [](void* context, Word dir) {
            auto self = static_cast<VIA*>(context);
            self->ifr &= ~0x40;  // Clear T1 interrupt flag on read
            self->UpdateIRQ();
            return (Byte)(self->t1c & 0xFF);
        },
        this);

    mem.SetWriteHook(
        T1C_H,
        [](void* context, Word dir, Byte val) {
            auto self = static_cast<VIA*>(context);
            self->t1c_h = val;
            self->t1l_h = val;
            self->t1l = (val << 8) | self->t1l_l;
            self->t1c = self->t1l;  // Load counter with latch
            self->ifr &= ~0x40;     // Clear T1 interrupt flag
            self->t1_active = true;
            self->t1_pb7_output =
                false;  // T1 output goes low on load (if enabled)

            // If PB7 output enabled, update pin
            if ((self->acr & 0x80) && (self->ddrb & 0x80)) {
                self->orb &= ~0x80;  // Low
                if (self->port_b_callback)
                    self->port_b_callback((self->irb & ~self->ddrb) |
                                          (self->orb & self->ddrb));
            }

            self->UpdateIRQ();
        },
        this);
    mem.SetReadHook(
        T1C_H,
        [](void* context, Word dir) {
            return (Byte)(static_cast<VIA*>(context)->t1c >> 8);
        },
        this);

    mem.SetWriteHook(
        T1L_L,
        [](void* context, Word dir, Byte val) {
            auto self = static_cast<VIA*>(context);
            self->t1l_l = val;
            self->t1l = (self->t1l & 0xFF00) | val;
        },
        this);
    mem.SetReadHook(
        T1L_L,
        [](void* context, Word dir) {
            return static_cast<VIA*>(context)->t1l_l;
        },
        this);

    mem.SetWriteHook(
        T1L_H,
        [](void* context, Word dir, Byte val) {
            auto self = static_cast<VIA*>(context);
            self->t1l_h = val;
            self->t1l = (val << 8) | self->t1l_l;
            self->ifr &= ~0x40;  // Clear T1 interrupt flag
            self->UpdateIRQ();
        },
        this);
    mem.SetReadHook(
        T1L_H,
        [](void* context, Word dir) {
            return static_cast<VIA*>(context)->t1l_h;
        },
        this);

    mem.SetWriteHook(
        T2C_L,
        [](void* context, Word dir, Byte val) {
            auto self = static_cast<VIA*>(context);
            self->t2l = (self->t2l & 0xFF00) | val;
            self->t2c_l = val;
        },
        this);
    mem.SetReadHook(
        T2C_L,
        [](void* context, Word dir) {
            auto self = static_cast<VIA*>(context);
            self->ifr &= ~0x20;  // Clear T2 interrupt flag on read
            self->UpdateIRQ();
            return (Byte)(self->t2c & 0xFF);
        },
        this);

    mem.SetWriteHook(
        T2C_H,
        [](void* context, Word dir, Byte val) {
            auto self = static_cast<VIA*>(context);
            self->t2c_h = val;
            self->t2l = (val << 8) | (self->t2l & 0xFF);
            self->t2c = self->t2l;
            self->ifr &= ~0x20;  // Clear T2 interrupt flag
            self->t2_active = true;
            self->UpdateIRQ();
        },
        this);
    mem.SetReadHook(
        T2C_H,
        [](void* context, Word dir) {
            return (Byte)(static_cast<VIA*>(context)->t2c >> 8);
        },
        this);

    // 0x600A - SR
    mem.SetWriteHook(
        SR,
        [](void* context, Word dir, Byte val) {
            auto self = static_cast<VIA*>(context);
            self->sr = val;
            self->ifr &= ~0x04;      // Clear SR interrupt bit
            self->sr_active = true;  // Start shifting (if mode enables it)
            self->sr_cnt = 0;        // Reset bit counter
            self->UpdateIRQ();
        },
        this);
    mem.SetReadHook(
        SR,
        [](void* context, Word dir) {
            auto self = static_cast<VIA*>(context);
            self->ifr &= ~0x04;  // Clear SR interrupt bit on read
            self->sr_active =
                true;  // Start shifting (if mode enables it, e.g. shifting in)
            self->sr_cnt = 0;
            self->UpdateIRQ();
            return self->sr;
        },
        this);

    // 0x600B - ACR
    mem.SetWriteHook(
        ACR,
        [](void* context, Word dir, Byte val) {
            static_cast<VIA*>(context)->acr = val;
        },
        this);
    mem.SetReadHook(
        ACR,
        [](void* context, Word dir) { return static_cast<VIA*>(context)->acr; },
        this);

    // 0x600C - PCR
    mem.SetWriteHook(
        PCR,
        [](void* context, Word dir, Byte val) {
            static_cast<VIA*>(context)->pcr = val;
        },
        this);
    mem.SetReadHook(
        PCR,
        [](void* context, Word dir) { return static_cast<VIA*>(context)->pcr; },
        this);

    // 0x600D - IFR
    // Typically IFR bits are cleared by writing 1 to them
    mem.SetWriteHook(
        IFR,
        [](void* context, Word dir, Byte val) {
            auto self = static_cast<VIA*>(context);
            // Only bits written as 1 are cleared. Bit 7 is top-level IRQ flag,
            // read-only.
            self->ifr &= ~(val & 0x7F);
            self->UpdateIRQ();
        },
        this);
    mem.SetReadHook(
        IFR,
        [](void* context, Word dir) { return static_cast<VIA*>(context)->ifr; },
        this);

    // 0x600E - IER
    mem.SetWriteHook(
        IER,
        [](void* context, Word dir, Byte val) {
            auto self = static_cast<VIA*>(context);
            if (val & 0x80) {
                self->ier |= (val & 0x7F);
            } else {
                self->ier &= ~(val & 0x7F);
            }
            self->ier &= 0x7F;  // Keep bit 7 always 0
            self->UpdateIRQ();
        },
        this);
    // Bit 7 always 1 on read
    mem.SetReadHook(
        IER,
        [](void* context, Word dir) {
            return (Byte)(static_cast<VIA*>(context)->ier | 0x80);
        },
        this);

    // 0x600F - ORA_NH (and ORA macro in Mem.h points to 0x600F)
    mem.SetWriteHook(
        ORA_NH,
        [](void* context, Word dir, Byte val) {
            auto self = static_cast<VIA*>(context);
            self->ora_nh = val;
            self->ora = val;  // Also update ORA
        },
        this);
    mem.SetReadHook(
        ORA_NH,
        [](void* context, Word dir) {
            return static_cast<VIA*>(context)->ora_nh;
        },
        this);
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

bool VIA::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&orb), sizeof(orb));
    out.write(reinterpret_cast<const char*>(&ora), sizeof(ora));
    out.write(reinterpret_cast<const char*>(&ddrb), sizeof(ddrb));
    out.write(reinterpret_cast<const char*>(&ddra), sizeof(ddra));
    out.write(reinterpret_cast<const char*>(&t1c_l), sizeof(t1c_l));
    out.write(reinterpret_cast<const char*>(&t1c_h), sizeof(t1c_h));
    out.write(reinterpret_cast<const char*>(&t1l_l), sizeof(t1l_l));
    out.write(reinterpret_cast<const char*>(&t1l_h), sizeof(t1l_h));
    out.write(reinterpret_cast<const char*>(&t2c_l), sizeof(t2c_l));
    out.write(reinterpret_cast<const char*>(&t2c_h), sizeof(t2c_h));
    out.write(reinterpret_cast<const char*>(&sr), sizeof(sr));
    out.write(reinterpret_cast<const char*>(&acr), sizeof(acr));
    out.write(reinterpret_cast<const char*>(&pcr), sizeof(pcr));
    out.write(reinterpret_cast<const char*>(&ifr), sizeof(ifr));
    out.write(reinterpret_cast<const char*>(&ier), sizeof(ier));
    out.write(reinterpret_cast<const char*>(&ora_nh), sizeof(ora_nh));
    out.write(reinterpret_cast<const char*>(&t1c), sizeof(t1c));
    out.write(reinterpret_cast<const char*>(&t1l), sizeof(t1l));
    out.write(reinterpret_cast<const char*>(&t2c), sizeof(t2c));
    out.write(reinterpret_cast<const char*>(&t2l), sizeof(t2l));
    out.write(reinterpret_cast<const char*>(&t1_active), sizeof(t1_active));
    out.write(reinterpret_cast<const char*>(&t2_active), sizeof(t2_active));
    out.write(reinterpret_cast<const char*>(&t1_pb7_output),
              sizeof(t1_pb7_output));
    out.write(reinterpret_cast<const char*>(&ira), sizeof(ira));
    out.write(reinterpret_cast<const char*>(&irb), sizeof(irb));
    out.write(reinterpret_cast<const char*>(&last_irb), sizeof(last_irb));
    out.write(reinterpret_cast<const char*>(&cb1_in), sizeof(cb1_in));
    out.write(reinterpret_cast<const char*>(&cb2_in), sizeof(cb2_in));
    out.write(reinterpret_cast<const char*>(&sr_cnt), sizeof(sr_cnt));
    out.write(reinterpret_cast<const char*>(&sr_active), sizeof(sr_active));
    out.write(reinterpret_cast<const char*>(&sr_out_cb2), sizeof(sr_out_cb2));
    return out.good();
}

bool VIA::LoadState(std::istream& in) {
    in.read(reinterpret_cast<char*>(&orb), sizeof(orb));
    in.read(reinterpret_cast<char*>(&ora), sizeof(ora));
    in.read(reinterpret_cast<char*>(&ddrb), sizeof(ddrb));
    in.read(reinterpret_cast<char*>(&ddra), sizeof(ddra));
    in.read(reinterpret_cast<char*>(&t1c_l), sizeof(t1c_l));
    in.read(reinterpret_cast<char*>(&t1c_h), sizeof(t1c_h));
    in.read(reinterpret_cast<char*>(&t1l_l), sizeof(t1l_l));
    in.read(reinterpret_cast<char*>(&t1l_h), sizeof(t1l_h));
    in.read(reinterpret_cast<char*>(&t2c_l), sizeof(t2c_l));
    in.read(reinterpret_cast<char*>(&t2c_h), sizeof(t2c_h));
    in.read(reinterpret_cast<char*>(&sr), sizeof(sr));
    in.read(reinterpret_cast<char*>(&acr), sizeof(acr));
    in.read(reinterpret_cast<char*>(&pcr), sizeof(pcr));
    in.read(reinterpret_cast<char*>(&ifr), sizeof(ifr));
    in.read(reinterpret_cast<char*>(&ier), sizeof(ier));
    in.read(reinterpret_cast<char*>(&ora_nh), sizeof(ora_nh));
    in.read(reinterpret_cast<char*>(&t1c), sizeof(t1c));
    in.read(reinterpret_cast<char*>(&t1l), sizeof(t1l));
    in.read(reinterpret_cast<char*>(&t2c), sizeof(t2c));
    in.read(reinterpret_cast<char*>(&t2l), sizeof(t2l));
    in.read(reinterpret_cast<char*>(&t1_active), sizeof(t1_active));
    in.read(reinterpret_cast<char*>(&t2_active), sizeof(t2_active));
    in.read(reinterpret_cast<char*>(&t1_pb7_output), sizeof(t1_pb7_output));
    in.read(reinterpret_cast<char*>(&ira), sizeof(ira));
    in.read(reinterpret_cast<char*>(&irb), sizeof(irb));
    in.read(reinterpret_cast<char*>(&last_irb), sizeof(last_irb));
    in.read(reinterpret_cast<char*>(&cb1_in), sizeof(cb1_in));
    in.read(reinterpret_cast<char*>(&cb2_in), sizeof(cb2_in));
    in.read(reinterpret_cast<char*>(&sr_cnt), sizeof(sr_cnt));
    in.read(reinterpret_cast<char*>(&sr_active), sizeof(sr_active));
    in.read(reinterpret_cast<char*>(&sr_out_cb2), sizeof(sr_out_cb2));
    return in.good();
}

}  // namespace Hardware
