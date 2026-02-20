#include "ACIA.h"

#include "Mem.h"

namespace Hardware {

void ACIA::Init(Mem& mem) {
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

}  // namespace Hardware
