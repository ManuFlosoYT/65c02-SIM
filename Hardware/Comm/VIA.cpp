#include "Hardware/Comm/VIA.h"

#include "Hardware/Core/Bus.h"

namespace Hardware {

VIA::VIA()
    : orb(0),
      ora(0),
      ddrb(0),
      ddra(0),
      t1c_l(0),
      t1c_h(0),
      t1l_l(0),
      t1l_h(0),
      t2c_l(0),
      t2c_h(0),
      sr(0),
      acr(0),
      pcr(0),
      ifr(0),
      ier(0),
      ora_nh(0),
      t1c(0xFFFF),
      t1l(0xFFFF),
      t2c(0xFFFF),
      t2l(0xFFFF),
      t1_active(false),
      t2_active(false),
      t1_pb7_output(true),
      ira(0x00),
      irb(0x00),
      last_irb(0x00),
      ca1_in(true),
      ca2_in(true),
      cb1_in(true),
      cb2_in(true),
      last_ca1(true),
      last_ca2(true),
      last_cb1(true),
      last_cb2(true),
      sr_cnt(0),
      sr_active(false),
      sr_out_cb2(true) {
    Reset();
}

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
    ca1_in = true;
    ca2_in = true;
    last_ca1 = true;
    last_ca2 = true;
    last_cb1 = true;
    last_cb2 = true;
    sr_cnt = 0;
    sr_active = false;
    sr_out_cb2 = true;
    UpdateAnyActive();
}

Byte VIA::Read(Word address) {
    switch (address & 0x0F) {
        case PORTB & 0x0F:
            ifr &= ~0x18;  // Clear CB1 and CB2 flags
            UpdateIRQ();
            return (irb & ~ddrb) | (orb & ddrb);
        case PORTA & 0x0F:
            ifr &= ~0x03;  // Clear CA1 and CA2 flags
            UpdateIRQ();
            return (ira & ~ddra) | (ora & ddra);
        case DDRB & 0x0F:
            return ddrb;
        case DDRA & 0x0F:
            return ddra;
        case T1C_L & 0x0F:
            ifr &= ~0x40;  // Clear T1 interrupt flag on read
            UpdateIRQ();
            return (Byte)(t1c & 0xFF);
        case T1C_H & 0x0F:
            return (Byte)(t1c >> 8);
        case T1L_L & 0x0F:
            return t1l_l;
        case T1L_H & 0x0F:
            return t1l_h;
        case T2C_L & 0x0F:
            ifr &= ~0x20;  // Clear T2 interrupt flag on read
            UpdateIRQ();
            return (Byte)(t2c & 0xFF);
        case T2C_H & 0x0F:
            return (Byte)(t2c >> 8);
        case SR & 0x0F:
            ifr &= ~0x04;  // Clear SR interrupt bit on read
            sr_active = true;
            sr_cnt = 0;
            UpdateAnyActive();
            UpdateIRQ();
            return sr;
        case ACR & 0x0F:
            return acr;
        case PCR & 0x0F:
            return pcr;
        case IFR & 0x0F:
            return ifr;
        case IER & 0x0F:
            return ier | 0x80;
        case ORA_NH & 0x0F:
            return ora_nh;
        default:
            return 0;
    }
}

void VIA::Write(Word address, Byte data) {
    switch (address & 0x0F) {
        case PORTB & 0x0F: {
            Byte old_pins = (irb & ~ddrb) | (orb & ddrb);
            orb = data;
            Byte new_pins = (irb & ~ddrb) | (orb & ddrb);
            if (port_b_callback && (old_pins != new_pins)) {
                port_b_callback(new_pins);
            }
            ifr &= ~0x18;  // Clear CB1 and CB2 flags
            UpdateIRQ();
            break;
        }
        case PORTA & 0x0F: {
            Byte old_pins = (ira & ~ddra) | (ora & ddra);
            ora = data;
            Byte new_pins = (ira & ~ddra) | (ora & ddra);
            if (port_a_callback && (old_pins != new_pins)) {
                port_a_callback(new_pins);
            }
            ifr &= ~0x03;  // Clear CA1 and CA2 flags
            UpdateIRQ();
            break;
        }
        case DDRB & 0x0F: {
            Byte old_pins = (irb & ~ddrb) | (orb & ddrb);
            ddrb = data;
            Byte new_pins = (irb & ~ddrb) | (orb & ddrb);
            if (port_b_callback && (old_pins != new_pins)) {
                port_b_callback(new_pins);
            }
            break;
        }
        case DDRA & 0x0F: {
            Byte old_pins = (ira & ~ddra) | (ora & ddra);
            ddra = data;
            Byte new_pins = (ira & ~ddra) | (ora & ddra);
            if (port_a_callback && (old_pins != new_pins)) {
                port_a_callback(new_pins);
            }
            break;
        }
        case T1C_L & 0x0F:
            t1l_l = data;
            break;
        case T1C_H & 0x0F:
            t1c_h = data;
            t1l_h = data;
            t1l = (data << 8) | t1l_l;
            t1c = t1l;  // Load counter with latch
            ifr &= ~0x40;
            t1_active = true;
            UpdateAnyActive();
            t1_pb7_output = false;
            if (((acr & 0x80) != 0) && ((ddrb & 0x80) != 0)) {
                orb &= ~0x80;
                if (port_b_callback) {
                    port_b_callback((irb & ~ddrb) | (orb & ddrb));
                }
            }
            UpdateIRQ();
            break;
        case T1L_L & 0x0F:
            t1l_l = data;
            t1l = (t1l & 0xFF00) | data;
            break;
        case T1L_H & 0x0F:
            t1l_h = data;
            t1l = (data << 8) | t1l_l;
            ifr &= ~0x40;
            UpdateIRQ();
            break;
        case T2C_L & 0x0F:
            t2l = (t2l & 0xFF00) | data;
            t2c_l = data;
            break;
        case T2C_H & 0x0F:
            t2c_h = data;
            t2l = (data << 8) | (t2l & 0xFF);
            t2c = t2l;
            ifr &= ~0x20;
            t2_active = true;
            UpdateAnyActive();
            UpdateIRQ();
            break;
        case SR & 0x0F:
            sr = data;
            ifr &= ~0x04;
            sr_active = true;
            UpdateAnyActive();
            sr_cnt = 0;
            UpdateIRQ();
            break;
        case ACR & 0x0F:
            acr = data;
            break;
        case PCR & 0x0F:
            pcr = data;
            break;
        case IFR & 0x0F:
            ifr &= ~(data & 0x7F);
            UpdateIRQ();
            break;
        case IER & 0x0F:
            if ((data & 0x80) != 0) {
                ier |= (data & 0x7F);
            } else {
                ier &= ~(data & 0x7F);
            }
            ier &= 0x7F;
            UpdateIRQ();
            break;
        case ORA_NH & 0x0F:
            ora_nh = data;
            ora = data;
            break;
        default:
            break;
    }
}

void VIA::UpdateIRQ() {
    if (((ifr & ier) & 0x7F) != 0) {
        ifr |= 0x80;  // Set IRQ bit if any enabled interrupt is active
    } else {
        ifr &= 0x7F;  // Clear IRQ bit
    }
}

Byte VIA::GetPortA() const { return (ira & ~ddra) | (ora & ddra); }
Byte VIA::GetPortB() const { return (irb & ~ddrb) | (orb & ddrb); }

void VIA::SetInputA(Byte val) { ira = val; }

void VIA::SetCA1(bool val) {
    bool old_ca1 = ca1_in;
    ca1_in = val;
    bool active_edge = false;
    if ((pcr & 0x01) == 0) {  // Negative edge
        if (old_ca1 && !val) {
            active_edge = true;
        }
    } else {  // Positive edge
        if (!old_ca1 && val) {
            active_edge = true;
        }
    }
    if (active_edge) {
        ifr |= 0x02;
        UpdateIRQ();
    }
}

void VIA::SetCA2(bool val) {
    bool old_ca2 = ca2_in;
    ca2_in = val;
    // CA2 control is bits 1-3 of PCR
    Byte ca2_mode = (pcr >> 1) & 0x07;
    bool active_edge = false;
    if (ca2_mode == 0 || ca2_mode == 1) {  // Input, negative edge or Independent input, neg edge
        if (old_ca2 && !val) {
            active_edge = true;
        }
    } else if (ca2_mode == 2 || ca2_mode == 3) {  // Input, positive edge or Independent input, pos edge
        if (!old_ca2 && val) {
            active_edge = true;
        }
    }
    // (Modes 4-7 are output modes)

    if (active_edge) {
        ifr |= 0x01;
        UpdateIRQ();
    }
}

void VIA::SetInputB(Byte val) {
    // Pulse Counting for T2 (PB6)
    // Check PB6 transition high-to-low
    if (((acr & 0x20) != 0) && t2_active) {
        // PB6 is bit 6
        bool old_pb6 = (irb & 0x40) != 0;
        bool new_pb6 = (val & 0x40) != 0;

        if (old_pb6 && !new_pb6) {  // Falling edge
            t2c--;
            if (t2c == 0xFFFF) {
                ifr |= 0x20;
                t2_active = false;
                UpdateAnyActive();
                UpdateIRQ();
            }
        }
    }

    irb = val;
}

void VIA::SetCB1(bool val) {
    bool old_cb1 = cb1_in;
    cb1_in = val;
    bool active_edge = false;
    if ((pcr & 0x10) == 0) {  // Negative edge
        if (old_cb1 && !val) {
            active_edge = true;
        }
    } else {  // Positive edge
        if (!old_cb1 && val) {
            active_edge = true;
        }
    }
    if (active_edge) {
        ifr |= 0x10;
        UpdateIRQ();
    }
}

void VIA::SetCB2(bool val) {
    bool old_cb2 = cb2_in;
    cb2_in = val;
    Byte cb2_mode = (pcr >> 5) & 0x07;
    bool active_edge = false;
    if (cb2_mode == 0 || cb2_mode == 1) {
        if (old_cb2 && !val) {
            active_edge = true;
        }
    } else if (cb2_mode == 2 || cb2_mode == 3) {
        if (!old_cb2 && val) {
            active_edge = true;
        }
    }
    if (active_edge) {
        ifr |= 0x08;
        UpdateIRQ();
    }
}

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
    out.write(reinterpret_cast<const char*>(&orb), sizeof(orb));                      // NOLINT
    out.write(reinterpret_cast<const char*>(&ora), sizeof(ora));                      // NOLINT
    out.write(reinterpret_cast<const char*>(&ddrb), sizeof(ddrb));                    // NOLINT
    out.write(reinterpret_cast<const char*>(&ddra), sizeof(ddra));                    // NOLINT
    out.write(reinterpret_cast<const char*>(&t1c_l), sizeof(t1c_l));                  // NOLINT
    out.write(reinterpret_cast<const char*>(&t1c_h), sizeof(t1c_h));                  // NOLINT
    out.write(reinterpret_cast<const char*>(&t1l_l), sizeof(t1l_l));                  // NOLINT
    out.write(reinterpret_cast<const char*>(&t1l_h), sizeof(t1l_h));                  // NOLINT
    out.write(reinterpret_cast<const char*>(&t2c_l), sizeof(t2c_l));                  // NOLINT
    out.write(reinterpret_cast<const char*>(&t2c_h), sizeof(t2c_h));                  // NOLINT
    out.write(reinterpret_cast<const char*>(&sr), sizeof(sr));                        // NOLINT
    out.write(reinterpret_cast<const char*>(&acr), sizeof(acr));                      // NOLINT
    out.write(reinterpret_cast<const char*>(&pcr), sizeof(pcr));                      // NOLINT
    out.write(reinterpret_cast<const char*>(&ifr), sizeof(ifr));                      // NOLINT
    out.write(reinterpret_cast<const char*>(&ier), sizeof(ier));                      // NOLINT
    out.write(reinterpret_cast<const char*>(&ora_nh), sizeof(ora_nh));                // NOLINT
    out.write(reinterpret_cast<const char*>(&t1c), sizeof(t1c));                      // NOLINT
    out.write(reinterpret_cast<const char*>(&t1l), sizeof(t1l));                      // NOLINT
    out.write(reinterpret_cast<const char*>(&t2c), sizeof(t2c));                      // NOLINT
    out.write(reinterpret_cast<const char*>(&t2l), sizeof(t2l));                      // NOLINT
    out.write(reinterpret_cast<const char*>(&t1_active), sizeof(t1_active));          // NOLINT
    out.write(reinterpret_cast<const char*>(&t2_active), sizeof(t2_active));          // NOLINT
    out.write(reinterpret_cast<const char*>(&t1_pb7_output), sizeof(t1_pb7_output));  // NOLINT
    out.write(reinterpret_cast<const char*>(&ira), sizeof(ira));                      // NOLINT
    out.write(reinterpret_cast<const char*>(&irb), sizeof(irb));                      // NOLINT
    out.write(reinterpret_cast<const char*>(&last_irb), sizeof(last_irb));            // NOLINT
    out.write(reinterpret_cast<const char*>(&ca1_in), sizeof(ca1_in));                // NOLINT
    out.write(reinterpret_cast<const char*>(&ca2_in), sizeof(ca2_in));                // NOLINT
    out.write(reinterpret_cast<const char*>(&cb1_in), sizeof(cb1_in));                // NOLINT
    out.write(reinterpret_cast<const char*>(&cb2_in), sizeof(cb2_in));                // NOLINT
    out.write(reinterpret_cast<const char*>(&last_ca1), sizeof(last_ca1));            // NOLINT
    out.write(reinterpret_cast<const char*>(&last_ca2), sizeof(last_ca2));            // NOLINT
    out.write(reinterpret_cast<const char*>(&last_cb1), sizeof(last_cb1));            // NOLINT
    out.write(reinterpret_cast<const char*>(&last_cb2), sizeof(last_cb2));            // NOLINT
    out.write(reinterpret_cast<const char*>(&sr_cnt), sizeof(sr_cnt));                // NOLINT
    out.write(reinterpret_cast<const char*>(&sr_active), sizeof(sr_active));          // NOLINT
    out.write(reinterpret_cast<const char*>(&sr_out_cb2), sizeof(sr_out_cb2));        // NOLINT
    return out.good();
}

bool VIA::LoadState(std::istream& inStream) {
    inStream.read(reinterpret_cast<char*>(&orb), sizeof(orb));                      // NOLINT
    inStream.read(reinterpret_cast<char*>(&ora), sizeof(ora));                      // NOLINT
    inStream.read(reinterpret_cast<char*>(&ddrb), sizeof(ddrb));                    // NOLINT
    inStream.read(reinterpret_cast<char*>(&ddra), sizeof(ddra));                    // NOLINT
    inStream.read(reinterpret_cast<char*>(&t1c_l), sizeof(t1c_l));                  // NOLINT
    inStream.read(reinterpret_cast<char*>(&t1c_h), sizeof(t1c_h));                  // NOLINT
    inStream.read(reinterpret_cast<char*>(&t1l_l), sizeof(t1l_l));                  // NOLINT
    inStream.read(reinterpret_cast<char*>(&t1l_h), sizeof(t1l_h));                  // NOLINT
    inStream.read(reinterpret_cast<char*>(&t2c_l), sizeof(t2c_l));                  // NOLINT
    inStream.read(reinterpret_cast<char*>(&t2c_h), sizeof(t2c_h));                  // NOLINT
    inStream.read(reinterpret_cast<char*>(&sr), sizeof(sr));                        // NOLINT
    inStream.read(reinterpret_cast<char*>(&acr), sizeof(acr));                      // NOLINT
    inStream.read(reinterpret_cast<char*>(&pcr), sizeof(pcr));                      // NOLINT
    inStream.read(reinterpret_cast<char*>(&ifr), sizeof(ifr));                      // NOLINT
    inStream.read(reinterpret_cast<char*>(&ier), sizeof(ier));                      // NOLINT
    inStream.read(reinterpret_cast<char*>(&ora_nh), sizeof(ora_nh));                // NOLINT
    inStream.read(reinterpret_cast<char*>(&t1c), sizeof(t1c));                      // NOLINT
    inStream.read(reinterpret_cast<char*>(&t1l), sizeof(t1l));                      // NOLINT
    inStream.read(reinterpret_cast<char*>(&t2c), sizeof(t2c));                      // NOLINT
    inStream.read(reinterpret_cast<char*>(&t2l), sizeof(t2l));                      // NOLINT
    inStream.read(reinterpret_cast<char*>(&t1_active), sizeof(t1_active));          // NOLINT
    inStream.read(reinterpret_cast<char*>(&t2_active), sizeof(t2_active));          // NOLINT
    inStream.read(reinterpret_cast<char*>(&t1_pb7_output), sizeof(t1_pb7_output));  // NOLINT
    inStream.read(reinterpret_cast<char*>(&ira), sizeof(ira));                      // NOLINT
    inStream.read(reinterpret_cast<char*>(&irb), sizeof(irb));                      // NOLINT
    inStream.read(reinterpret_cast<char*>(&last_irb), sizeof(last_irb));            // NOLINT
    inStream.read(reinterpret_cast<char*>(&ca1_in), sizeof(ca1_in));                // NOLINT
    inStream.read(reinterpret_cast<char*>(&ca2_in), sizeof(ca2_in));                // NOLINT
    inStream.read(reinterpret_cast<char*>(&cb1_in), sizeof(cb1_in));                // NOLINT
    inStream.read(reinterpret_cast<char*>(&cb2_in), sizeof(cb2_in));                // NOLINT
    inStream.read(reinterpret_cast<char*>(&last_ca1), sizeof(last_ca1));            // NOLINT
    inStream.read(reinterpret_cast<char*>(&last_ca2), sizeof(last_ca2));            // NOLINT
    inStream.read(reinterpret_cast<char*>(&last_cb1), sizeof(last_cb1));            // NOLINT
    inStream.read(reinterpret_cast<char*>(&last_cb2), sizeof(last_cb2));            // NOLINT
    inStream.read(reinterpret_cast<char*>(&sr_cnt), sizeof(sr_cnt));                // NOLINT
    inStream.read(reinterpret_cast<char*>(&sr_active), sizeof(sr_active));          // NOLINT
    inStream.read(reinterpret_cast<char*>(&sr_out_cb2), sizeof(sr_out_cb2));        // NOLINT
    UpdateAnyActive();
    UpdateIRQ();
    return inStream.good();
}

}  // namespace Hardware
