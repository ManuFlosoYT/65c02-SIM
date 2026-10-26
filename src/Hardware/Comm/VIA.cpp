#include "Hardware/Comm/VIA.h"

#include "Hardware/Core/Bus.h"

namespace Hardware {

VIA::VIA() { Reset(); }

void VIA::Reset() {
    orb = 0;
    ora = 0;
    ddrb = 0;
    ddra = 0;
    acr = 0;
    pcr = 0;
    ifr = 0;
    ier = 0;

    timer1.Reset();
    timer2.Reset();
    shiftRegister.Reset();

    ira = 0x00;
    irb = 0x00;
    cb1_in = true;
    cb2_in = true;
    ca1_in = true;
    ca2_in = true;
    ira_latched = 0;
    irb_latched = 0;
    ira_latch_full = false;
    irb_latch_full = false;
    ca2_out = true;
    cb1_out = true;
    cb2_out = true;
    ca2_pulse_pending = false;
    cb1_pulse_pending = false;
    cb2_pulse_pending = false;
    UpdateAnyActive();
}

Byte VIA::Read(Word address) {
    switch (address & 0x0F) {
        case PORTB & 0x0F: {
            Byte cb2_mode = (pcr >> 5) & 0x07;
            if (cb2_mode == 1 || cb2_mode == 3) {
                ifr &= ~0x10;  // Clear CB1 only
            } else {
                ifr &= ~0x18;  // Clear CB1 and CB2 flags
            }
            UpdateIRQ();
            
            Byte val = irb;
            if ((acr & 0x02) != 0 && irb_latch_full) {
                val = irb_latched;
            }
            irb_latch_full = false;
            return (val & ~ddrb) | (GetEffectiveORB() & ddrb);
        }
        case PORTA & 0x0F: {
            Byte ca2_mode = (pcr >> 1) & 0x07;
            if (ca2_mode == 1 || ca2_mode == 3) {
                ifr &= ~0x02;  // Clear CA1 only
            } else {
                ifr &= ~0x03;  // Clear CA1 and CA2 flags
            }
            UpdateIRQ();
            
            if (ca2_mode == 4) {
                UpdateCA2Output(false);
            } else if (ca2_mode == 5) {
                UpdateCA2Output(false);
                ca2_pulse_pending = true;
                UpdateAnyActive();
            }
            
            Byte val = (ira & ~ddra) | (ora & ddra);
            if ((acr & 0x01) != 0 && ira_latch_full) {
                val = ira_latched;
            }
            ira_latch_full = false;
            return val;
        }
        case DDRB & 0x0F:
            return ddrb;
        case DDRA & 0x0F:
            return ddra;
        case T1C_L & 0x0F:
            ifr &= ~0x40;  // Clear T1 interrupt flag on read
            UpdateIRQ();
            return timer1.ReadLow();
        case T1C_H & 0x0F:
            return timer1.ReadHigh();
        case T1L_L & 0x0F:
            return timer1.ReadLatchLow();
        case T1L_H & 0x0F:
            return timer1.ReadLatchHigh();
        case T2C_L & 0x0F:
            ifr &= ~0x20;  // Clear T2 interrupt flag on read
            UpdateIRQ();
            return timer2.ReadLow();
        case T2C_H & 0x0F:
            return timer2.ReadHigh();
        case VIA_SR & 0x0F:
            ifr &= ~0x04;  // Clear SR interrupt bit on read
            {
                Byte val = shiftRegister.Read();
                UpdateAnyActive();
                UpdateIRQ();
                return val;
            }
        case ACR & 0x0F:
            return acr;
        case PCR & 0x0F:
            return pcr;
        case IFR & 0x0F:
            return ifr;
        case IER & 0x0F:
            return ier | 0x80;
        case ORA_NH & 0x0F: {
            Byte val = (ira & ~ddra) | (ora & ddra);
            if ((acr & 0x01) != 0 && ira_latch_full) {
                val = ira_latched;
            }
            return val;
        }
        default:
            return 0;
    }
}

Byte VIA::ReadDirect(Word address) {
    switch (address & 0x0F) {
        case PORTB & 0x0F: {
            Byte val = irb;
            if ((acr & 0x02) != 0 && irb_latch_full) {
                val = irb_latched;
            }
            return (val & ~ddrb) | (GetEffectiveORB() & ddrb);
        }
        case PORTA & 0x0F:
        case ORA_NH & 0x0F: {
            Byte val = (ira & ~ddra) | (ora & ddra);
            if ((acr & 0x01) != 0 && ira_latch_full) {
                val = ira_latched;
            }
            return val;
        }
        case DDRB & 0x0F:
            return ddrb;
        case DDRA & 0x0F:
            return ddra;
        case T1C_L & 0x0F:
            return timer1.ReadLow();
        case T1C_H & 0x0F:
            return timer1.ReadHigh();
        case T1L_L & 0x0F:
            return timer1.ReadLatchLow();
        case T1L_H & 0x0F:
            return timer1.ReadLatchHigh();
        case T2C_L & 0x0F:
            return timer2.ReadLow();
        case T2C_H & 0x0F:
            return timer2.ReadHigh();
        case VIA_SR & 0x0F:
            return shiftRegister.sr;
        case ACR & 0x0F:
            return acr;
        case PCR & 0x0F:
            return pcr;
        case IFR & 0x0F:
            return ifr;
        case IER & 0x0F:
            return ier | 0x80;
        default:
            return 0;
    }
}

void VIA::Write(Word address, Byte data) {
    switch (address & 0x0F) {
        case PORTB & 0x0F: {
            Byte old_pins = (irb & ~ddrb) | (GetEffectiveORB() & ddrb);
            orb = data;
            irb_latch_full = false;
            Byte new_pins = (irb & ~ddrb) | (GetEffectiveORB() & ddrb);
            if (port_b_callback && (old_pins != new_pins)) {
                port_b_callback(new_pins);
            }
            Byte cb2_mode = (pcr >> 5) & 0x07;
            if (cb2_mode == 1 || cb2_mode == 3) {
                ifr &= ~0x10;  // Clear CB1 only
            } else {
                ifr &= ~0x18;  // Clear CB1 and CB2 flags
            }
            UpdateIRQ();
            
            if (cb2_mode == 4) {
                UpdateCB2Output(false);
            } else if (cb2_mode == 5) {
                UpdateCB2Output(false);
                cb2_pulse_pending = true;
                UpdateAnyActive();
            }
            break;
        }
        case PORTA & 0x0F: {
            ora = data;
            ira_latch_full = false;
            Byte ca2_mode = (pcr >> 1) & 0x07;
            if (ca2_mode == 1 || ca2_mode == 3) {
                ifr &= ~0x02;  // Clear CA1 only
            } else {
                ifr &= ~0x03;  // Clear CA1 and CA2 flags
            }
            UpdateIRQ();

            if (ca2_mode == 4) {
                UpdateCA2Output(false);
            } else if (ca2_mode == 5) {
                UpdateCA2Output(false);
                ca2_pulse_pending = true;
                UpdateAnyActive();
            }
            break;
        }
        case DDRB & 0x0F: {
            Byte old_pins = (irb & ~ddrb) | (GetEffectiveORB() & ddrb);
            ddrb = data;
            Byte new_pins = (irb & ~ddrb) | (GetEffectiveORB() & ddrb);
            if (port_b_callback && (old_pins != new_pins)) {
                port_b_callback(new_pins);
            }
            break;
        }
        case DDRA & 0x0F:
            ddra = data;
            break;
        case T1C_L & 0x0F:
            timer1.WriteCounterLow(data);
            break;
        case T1C_H & 0x0F:
            timer1.WriteCounterHigh(data);
            ifr &= ~0x40;
            UpdateAnyActive();
            if (((acr & 0x80) != 0) && ((ddrb & 0x80) != 0)) {
                if (port_b_callback) {
                    port_b_callback((irb & ~ddrb) | (GetEffectiveORB() & ddrb));
                }
            }
            UpdateIRQ();
            break;
        case T1L_L & 0x0F:
            timer1.WriteLatchLow(data);
            break;
        case T1L_H & 0x0F:
            timer1.WriteLatchHigh(data);
            ifr &= ~0x40;
            UpdateIRQ();
            break;
        case T2C_L & 0x0F:
            timer2.WriteLow(data);
            break;
        case T2C_H & 0x0F:
            timer2.WriteHigh(data);
            ifr &= ~0x20;
            UpdateAnyActive();
            UpdateIRQ();
            break;
        case VIA_SR & 0x0F:
            shiftRegister.Write(data);
            ifr &= ~0x04;
            UpdateAnyActive();
            UpdateIRQ();
            break;
        case ACR & 0x0F: {
            Byte old_acr = acr;
            Byte old_pins = (irb & ~ddrb) | (GetEffectiveORB() & ddrb);
            acr = data;
            Byte new_pins = (irb & ~ddrb) | (GetEffectiveORB() & ddrb);
            if (port_b_callback && (old_pins != new_pins)) {
                port_b_callback(new_pins);
            }
            UpdateAnyActive();
            if ((old_acr & 0x01) != 0 && (acr & 0x01) == 0) {
                ira_latch_full = false;
            }
            if ((old_acr & 0x02) != 0 && (acr & 0x02) == 0) {
                irb_latch_full = false;
            }
            break;
        }
        case PCR & 0x0F: {
            pcr = data;
            Byte ca2_mode = (pcr >> 1) & 0x07;
            if (ca2_mode == 6) UpdateCA2Output(false);
            else if (ca2_mode == 7) UpdateCA2Output(true);

            Byte cb2_mode = (pcr >> 5) & 0x07;
            if (cb2_mode == 6) UpdateCB2Output(false);
            else if (cb2_mode == 7) UpdateCB2Output(true);
            break;
        }
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
            ora = data;
            break;
        default:
            break;
    }
}

void VIA::ClockSlow() {
    bool need_update = false;
    if (ca2_pulse_pending || cb1_pulse_pending || cb2_pulse_pending) [[unlikely]] {
        if (ca2_pulse_pending) {
            UpdateCA2Output(true);
            ca2_pulse_pending = false;
        }
        if (cb1_pulse_pending) {
            UpdateCB1Output(true);
            cb1_pulse_pending = false;
        }
        if (cb2_pulse_pending) {
            UpdateCB2Output(true);
            cb2_pulse_pending = false;
        }
        need_update = true;
    }

    const bool t1_underflowed = (timer1.t1c == 0);
    HandleTimer1();
    const bool t2_underflowed = HandleTimer2();
    if (shiftRegister.sr_active) [[unlikely]] {
        HandleShiftRegister(t2_underflowed);
        need_update = true;
    }
    if (need_update || t1_underflowed || t2_underflowed) [[unlikely]] {
        UpdateAnyActive();
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
Byte VIA::GetPortB() const { return (irb & ~ddrb) | (GetEffectiveORB() & ddrb); }

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
        if ((acr & 0x01) != 0) {
            ira_latched = (ira & ~ddra) | (ora & ddra);
            ira_latch_full = true;
        }
        Byte ca2_mode = (pcr >> 1) & 0x07;
        if (ca2_mode == 4) {
            UpdateCA2Output(true);
        }
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
    if (((acr & 0x20) != 0)) {
        bool old_pb6 = (irb & 0x40) != 0;
        bool new_pb6 = (val & 0x40) != 0;

        if (old_pb6 && !new_pb6) {  // Falling edge
            auto res = timer2.PulsePB6(acr);
            if (res.underflow) {
                if (res.interruptTriggered) {
                    ifr |= 0x20;
                    UpdateIRQ();
                }
                HandleShiftRegister(true);
                UpdateAnyActive();
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
        if ((acr & 0x02) != 0) {
            irb_latched = irb;
            irb_latch_full = true;
        }
        Byte cb2_mode = (pcr >> 5) & 0x07;
        if (cb2_mode == 4) {
            UpdateCB2Output(true); // Handshake restored by CB1 active edge
        }
        ifr |= 0x10;
        
        if (shiftRegister.sr_active) {
            auto res = shiftRegister.ShiftExternalCB1(acr, cb2_in);
            if (res.cb2Updated) {
                UpdateCB2Output(res.cb2Out);
            }
            if (res.interruptTriggered) {
                ifr |= 0x04;
                UpdateAnyActive();
            }
        }
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

Byte VIA::GetDDRB() const { return ddrb; }
Byte VIA::GetT1L_L() const { return timer1.t1l_l; }

bool VIA::SaveState(std::ostream& out) const {
    ISerializable::Serialize(out, orb);
    ISerializable::Serialize(out, ora);
    ISerializable::Serialize(out, ddrb);
    ISerializable::Serialize(out, ddra);
    ISerializable::Serialize(out, timer1.t1l_l);
    ISerializable::Serialize(out, timer1.t1l_h);
    ISerializable::Serialize(out, shiftRegister.sr);
    ISerializable::Serialize(out, acr);
    ISerializable::Serialize(out, pcr);
    ISerializable::Serialize(out, ifr);
    ISerializable::Serialize(out, ier);
    ISerializable::Serialize(out, timer1.t1c);
    ISerializable::Serialize(out, timer1.t1l);
    ISerializable::Serialize(out, timer2.t2c);
    ISerializable::Serialize(out, timer2.t2l);
    ISerializable::Serialize(out, timer1.t1Active);
    ISerializable::Serialize(out, timer2.t2_interrupt_armed);
    ISerializable::Serialize(out, timer1.t1_pb7_output);
    ISerializable::Serialize(out, ira);
    ISerializable::Serialize(out, irb);
    ISerializable::Serialize(out, ca1_in);
    ISerializable::Serialize(out, ca2_in);
    ISerializable::Serialize(out, cb1_in);
    ISerializable::Serialize(out, cb2_in);
    ISerializable::Serialize(out, shiftRegister.sr_cnt);
    ISerializable::Serialize(out, shiftRegister.sr_active);
    ISerializable::Serialize(out, shiftRegister.sr_out_cb2);
    ISerializable::Serialize(out, ira_latched);
    ISerializable::Serialize(out, irb_latched);
    ISerializable::Serialize(out, ira_latch_full);
    ISerializable::Serialize(out, irb_latch_full);
    ISerializable::Serialize(out, ca2_out);
    ISerializable::Serialize(out, cb1_out);
    ISerializable::Serialize(out, cb2_out);
    ISerializable::Serialize(out, ca2_pulse_pending);
    ISerializable::Serialize(out, cb1_pulse_pending);
    ISerializable::Serialize(out, cb2_pulse_pending);
    return out.good();
}

bool VIA::LoadState(std::istream& inStream) {
    ISerializable::Deserialize(inStream, orb);
    ISerializable::Deserialize(inStream, ora);
    ISerializable::Deserialize(inStream, ddrb);
    ISerializable::Deserialize(inStream, ddra);
    ISerializable::Deserialize(inStream, timer1.t1l_l);
    ISerializable::Deserialize(inStream, timer1.t1l_h);
    ISerializable::Deserialize(inStream, shiftRegister.sr);
    ISerializable::Deserialize(inStream, acr);
    ISerializable::Deserialize(inStream, pcr);
    ISerializable::Deserialize(inStream, ifr);
    ISerializable::Deserialize(inStream, ier);
    ISerializable::Deserialize(inStream, timer1.t1c);
    ISerializable::Deserialize(inStream, timer1.t1l);
    ISerializable::Deserialize(inStream, timer2.t2c);
    ISerializable::Deserialize(inStream, timer2.t2l);
    ISerializable::Deserialize(inStream, timer1.t1Active);
    ISerializable::Deserialize(inStream, timer2.t2_interrupt_armed);
    ISerializable::Deserialize(inStream, timer1.t1_pb7_output);
    ISerializable::Deserialize(inStream, ira);
    ISerializable::Deserialize(inStream, irb);
    ISerializable::Deserialize(inStream, ca1_in);
    ISerializable::Deserialize(inStream, ca2_in);
    ISerializable::Deserialize(inStream, cb1_in);
    ISerializable::Deserialize(inStream, cb2_in);
    ISerializable::Deserialize(inStream, shiftRegister.sr_cnt);
    ISerializable::Deserialize(inStream, shiftRegister.sr_active);
    ISerializable::Deserialize(inStream, shiftRegister.sr_out_cb2);
    ISerializable::Deserialize(inStream, ira_latched);
    ISerializable::Deserialize(inStream, irb_latched);
    ISerializable::Deserialize(inStream, ira_latch_full);
    ISerializable::Deserialize(inStream, irb_latch_full);
    ISerializable::Deserialize(inStream, ca2_out);
    ISerializable::Deserialize(inStream, cb1_out);
    ISerializable::Deserialize(inStream, cb2_out);
    ISerializable::Deserialize(inStream, ca2_pulse_pending);
    ISerializable::Deserialize(inStream, cb1_pulse_pending);
    ISerializable::Deserialize(inStream, cb2_pulse_pending);
    UpdateAnyActive();
    UpdateIRQ();
    return inStream.good();
}

}  // namespace Hardware
