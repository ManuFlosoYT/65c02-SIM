#pragma once

#include <cstdint>
#include <string>

#include "Hardware/Core/Bus.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"

namespace Hardware {

struct DecodedInstruction {
    uint16_t address = 0;
    uint8_t opcode = 0;
    uint8_t size = 1;
    AddressingMode mode = AddressingMode::Implied;
    uint16_t operandValue = 0;
    bool hasOperand = false;
    bool isDataByte = false;
};

struct DisassembledInstruction {
    std::string address;
    std::string bytes;
    std::string mnemonic;
    std::string operands;
    uint16_t size = 1;
    uint16_t operandValue = 0;
    bool hasOperand = false;
    AddressingMode mode = AddressingMode::Implied;
};

class Disassembler {
public:
    static DecodedInstruction DecodeInstruction(const Bus& bus, uint16_t addr);
    static DisassembledInstruction Disassemble(const Bus& bus, uint16_t addr);
};

}  // namespace Hardware
