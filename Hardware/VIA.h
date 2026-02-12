#ifndef SIM_65C02_VIA_H
#define SIM_65C02_VIA_H

#include "Mem.h"

class VIA {
public:
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

    VIA();
    void Init(Mem& mem);
    void Reset();

    Byte GetPortA() const;
    Byte GetPortB() const;
    void SetPortA(Byte val);
    void SetPortB(Byte val);

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

private:
    // Internal state if needed (e.g., latches, timers running status)
};

#endif  // SIM_65C02_VIA_H
