#pragma once

#include <string>

#include "../Mem.h"

namespace Hardware {

struct DisassembledInstruction {
    std::string address;
    std::string bytes;
    std::string mnemonic;
    std::string operands;
    uint16_t size;
};

class Disassembler {
public:
    static DisassembledInstruction Disassemble(const Mem& mem, uint16_t addr);
};

}  // namespace Hardware
