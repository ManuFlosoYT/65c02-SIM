#pragma once

#include <string>

#include "Hardware/Core/Bus.h"

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
    static DisassembledInstruction Disassemble(const Bus& bus, uint16_t addr);
};

}  // namespace Hardware
