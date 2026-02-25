// CPU.inl — Inline implementations for Hardware::CPU

inline void Hardware::CPU::Reset() {
    PC = 0xFFFC;
    SP = 0x01FF;

    A = 0;
    X = 0;
    Y = 0;

    C = 0;
    Z = 0;
    I = 0;
    D = 0;
    B = 0;
    V = 0;
    N = 0;
    isInit = false;
    waiting = false;
    remainingCycles = 0;
}

inline int Hardware::CPU::Execute(Bus& bus) {
    while (true) {
        int res = Step(bus);
        if (res != 0) return res;
    }
    return 0;
}

inline int Hardware::CPU::Step(Bus& bus) {
    if (!isInit) {
        PC = ReadWord(0xFFFC, bus);
        isInit = true;
    }

    if (waiting) {
        if ((bus.Read(ACIA_STATUS) & 0x80) != 0) {
            waiting = false;
        } else {
            return 0;
        }
    }

    if (cycleAccurate && remainingCycles > 0) {
        remainingCycles--;
        return 0;
    }

    return Dispatch(bus);
}

inline void Hardware::CPU::IRQ(Bus& bus) {
    waiting = false;
    if (!I) {
        PushWord(PC, bus);
        B = 0;
        PushByte(GetStatus(), bus);
        I = 1;
        D = 0;
        PC = ReadWord(0xFFFE, bus);
    }
}

inline int Hardware::CPU::Dispatch(Bus& bus) {
    return CPUDispatch::Dispatch(*this, bus);
}

inline const Hardware::Byte Hardware::CPU::GetStatus() const {
    Byte status = 0;
    status |= C;
    status |= Z << 1;
    status |= I << 2;
    status |= D << 3;
    status |= B << 4;
    status |= V << 6;
    status |= N << 7;
    return status;
}

inline void Hardware::CPU::SetStatus(Byte status) {
    C = status & 0x01;
    Z = (status >> 1) & 0x01;
    I = (status >> 2) & 0x01;
    D = (status >> 3) & 0x01;
    B = (status >> 4) & 0x01;
    V = (status >> 6) & 0x01;
    N = (status >> 7) & 0x01;
}

inline bool Hardware::CPU::IsCycleAccurate() const { return cycleAccurate; }
inline void Hardware::CPU::SetCycleAccurate(bool enabled) {
    cycleAccurate = enabled;
}
inline int Hardware::CPU::GetRemainingCycles() const { return remainingCycles; }

inline void Hardware::CPU::AddPageCrossPenalty(Word baseAddr,
                                               Word effectiveAddr) {
    if (cycleAccurate && ((baseAddr & 0xFF00) != (effectiveAddr & 0xFF00))) {
        remainingCycles++;
    }
}

inline const Hardware::Byte Hardware::CPU::FetchByte(Bus& bus) {
    Byte dato = bus.Read(PC);
    PC++;
    return dato;
}

inline const Hardware::Word Hardware::CPU::FetchWord(Bus& bus) {
    Word dato = bus.Read(PC);
    dato |= (bus.Read(PC + 1) << 8);
    PC += 2;
    return dato;
}

inline const Hardware::Byte Hardware::CPU::ReadByte(const Word addr, Bus& bus) {
    return bus.Read(addr);
}

inline const Hardware::Word Hardware::CPU::ReadWord(const Word addr, Bus& bus) {
    Word dato = bus.Read(addr);
    dato |= (bus.Read(addr + 1) << 8);
    return dato;
}

inline void Hardware::CPU::PushByte(Byte val, Bus& bus) {
    bus.Write(SP, val);
    SP--;
    if (SP < 0x0100) SP = 0x01FF;
}

inline Hardware::Byte Hardware::CPU::PopByte(Bus& bus) {
    SP++;
    if (SP > 0x01FF) SP = 0x0100;
    return bus.Read(SP);
}

inline void Hardware::CPU::PushWord(Word val, Bus& bus) {
    PushByte((val >> 8) & 0xFF, bus);
    PushByte(val & 0xFF, bus);
}

inline Hardware::Word Hardware::CPU::PopWord(Bus& bus) {
    Word Low = PopByte(bus);
    Word High = PopByte(bus);
    return (High << 8) | Low;
}
