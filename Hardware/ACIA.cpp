#include "ACIA.h"

#include "Mem.h"

namespace Hardware {

void ACIA::Init(Mem& mem) {
    DATA = 0;
    STATUS = 0;
    CMD = 0;
    CTRL = 0;
    mem.SetWriteHook(
        ACIA_DATA,
        [](void* context, Word dir, Byte val) {
            auto self = static_cast<ACIA*>(context);
            self->DATA = val;
            if (self->outputCallback) {
                self->outputCallback((char)val);
            }
        },
        this);

    mem.SetWriteHook(
        ACIA_STATUS,
        [](void* context, Word dir, Byte val) {
            static_cast<ACIA*>(context)->STATUS = val;
        },
        this);

    mem.SetWriteHook(
        ACIA_CMD,
        [](void* context, Word dir, Byte val) {
            static_cast<ACIA*>(context)->CMD = val;
        },
        this);

    mem.SetWriteHook(
        ACIA_CTRL,
        [](void* context, Word dir, Byte val) {
            static_cast<ACIA*>(context)->CTRL = val;
        },
        this);

    struct AciaReadCtx {
        ACIA* acia;
        Mem* mem;
    };
    static AciaReadCtx ctx{this, &mem};

    mem.SetReadHook(
        ACIA_DATA,
        [](void* context, Word dir) {
            auto c = static_cast<AciaReadCtx*>(context);
            c->mem->memory[ACIA_STATUS] &= ~0x80;
            return c->mem->memory[ACIA_DATA];
        },
        &ctx);
}

bool ACIA::SaveState(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&DATA), sizeof(DATA));
    out.write(reinterpret_cast<const char*>(&STATUS), sizeof(STATUS));
    out.write(reinterpret_cast<const char*>(&CMD), sizeof(CMD));
    out.write(reinterpret_cast<const char*>(&CTRL), sizeof(CTRL));
    return out.good();
}

bool ACIA::LoadState(std::istream& in) {
    in.read(reinterpret_cast<char*>(&DATA), sizeof(DATA));
    in.read(reinterpret_cast<char*>(&STATUS), sizeof(STATUS));
    in.read(reinterpret_cast<char*>(&CMD), sizeof(CMD));
    in.read(reinterpret_cast<char*>(&CTRL), sizeof(CTRL));
    return in.good();
}

}  // namespace Hardware
