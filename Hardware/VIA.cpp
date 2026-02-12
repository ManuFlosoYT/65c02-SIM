#include "VIA.h"

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
}

void VIA::Init(Mem& mem) {
    // 0x6000 - ORB / IRB (PORTB)
    mem.SetWriteHook(PORTB, [this](Word dir, Byte val) { this->orb = val; });
    mem.SetReadHook(PORTB, [this](Word dir) { return this->orb; });

    // 0x6001 - ORA / IRA (PORTA)
    mem.SetWriteHook(PORTA, [this](Word dir, Byte val) { this->ora = val; });
    mem.SetReadHook(PORTA, [this](Word dir) { return this->ora; });

    // 0x6002 - DDRB
    mem.SetWriteHook(DDRB, [this](Word dir, Byte val) { this->ddrb = val; });
    mem.SetReadHook(DDRB, [this](Word dir) { return this->ddrb; });

    // 0x6003 - DDRA
    mem.SetWriteHook(DDRA, [this](Word dir, Byte val) { this->ddra = val; });
    mem.SetReadHook(DDRA, [this](Word dir) { return this->ddra; });

    // 0x6004 - T1C_L
    mem.SetWriteHook(T1C_L, [this](Word dir, Byte val) { this->t1c_l = val; });
    mem.SetReadHook(T1C_L, [this](Word dir) { return this->t1c_l; });

    // 0x6005 - T1C_H
    mem.SetWriteHook(T1C_H, [this](Word dir, Byte val) { this->t1c_h = val; });
    mem.SetReadHook(T1C_H, [this](Word dir) { return this->t1c_h; });

    // 0x6006 - T1L_L
    mem.SetWriteHook(T1L_L, [this](Word dir, Byte val) { this->t1l_l = val; });
    mem.SetReadHook(T1L_L, [this](Word dir) { return this->t1l_l; });

    // 0x6007 - T1L_H
    mem.SetWriteHook(T1L_H, [this](Word dir, Byte val) { this->t1l_h = val; });
    mem.SetReadHook(T1L_H, [this](Word dir) { return this->t1l_h; });

    // 0x6008 - T2C_L
    mem.SetWriteHook(T2C_L, [this](Word dir, Byte val) { this->t2c_l = val; });
    mem.SetReadHook(T2C_L, [this](Word dir) { return this->t2c_l; });

    // 0x6009 - T2C_H
    mem.SetWriteHook(T2C_H, [this](Word dir, Byte val) { this->t2c_h = val; });
    mem.SetReadHook(T2C_H, [this](Word dir) { return this->t2c_h; });

    // 0x600A - SR
    mem.SetWriteHook(SR, [this](Word dir, Byte val) { this->sr = val; });
    mem.SetReadHook(SR, [this](Word dir) { return this->sr; });

    // 0x600B - ACR
    mem.SetWriteHook(ACR, [this](Word dir, Byte val) { this->acr = val; });
    mem.SetReadHook(ACR, [this](Word dir) { return this->acr; });

    // 0x600C - PCR
    mem.SetWriteHook(PCR, [this](Word dir, Byte val) { this->pcr = val; });
    mem.SetReadHook(PCR, [this](Word dir) { return this->pcr; });

    // 0x600D - IFR
    // Typically IFR bits are cleared by writing 1 to them
    mem.SetWriteHook(IFR, [this](Word dir, Byte val) { this->ifr = val; });
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

Byte VIA::GetPortA() const { return ora; }
Byte VIA::GetPortB() const { return orb; }

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
