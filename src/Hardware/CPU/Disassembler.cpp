#include "Disassembler.h"

#include <iomanip>
#include <sstream>

#include "Hardware/CPU/Instructions/InstructionSet.h"

namespace Hardware {

DecodedInstruction Disassembler::DecodeInstruction(const Bus& bus, uint16_t addr) {
    DecodedInstruction inst;
    inst.address = addr;
    inst.size = 1;

    uint8_t flags = bus.GetCDL(addr);
    bool isOpcode = (flags & CDL_OPCODE) != 0;
    bool isData = (flags & (CDL_DATA | CDL_ZP_POINTER | CDL_OPERAND)) != 0;

    if (!isOpcode && isData) {
        inst.opcode = bus.ReadDirect(addr);
        inst.isDataByte = true;
        inst.hasOperand = false;
        inst.operandValue = inst.opcode;
        inst.mode = AddressingMode::Implied;
        return inst;
    }

    inst.opcode = bus.ReadDirect(addr);
    const InstructionInfo& info = OpcodeTable.at(inst.opcode);
    inst.mode = info.mode;

    switch (info.mode) {
        case AddressingMode::Implied:
        case AddressingMode::Accumulator:
            inst.size = 1;
            inst.hasOperand = false;
            inst.operandValue = 0;
            break;
        case AddressingMode::Immediate:
        case AddressingMode::ZeroPage:
        case AddressingMode::ZeroPageX:
        case AddressingMode::ZeroPageY:
        case AddressingMode::IndirectX:
        case AddressingMode::IndirectY:
        case AddressingMode::ZeroPageIndirect:
            inst.size = 2;
            inst.hasOperand = true;
            inst.operandValue = bus.ReadDirect(addr + 1);
            break;
        case AddressingMode::Absolute:
        case AddressingMode::AbsoluteX:
        case AddressingMode::AbsoluteY:
        case AddressingMode::Indirect:
        case AddressingMode::AbsoluteIndexedIndirect: {
            inst.size = 3;
            inst.hasOperand = true;
            uint8_t low = bus.ReadDirect(addr + 1);
            uint8_t high = bus.ReadDirect(addr + 2);
            inst.operandValue = static_cast<uint16_t>(low | (high << 8));
            break;
        }
        case AddressingMode::Relative: {
            inst.size = 2;
            inst.hasOperand = true;
            auto offset = static_cast<int8_t>(bus.ReadDirect(addr + 1));
            inst.operandValue = static_cast<uint16_t>(addr + 2 + offset);
            break;
        }
        case AddressingMode::ZeroPageRelative: {
            inst.size = 3;
            inst.hasOperand = true;
            auto rel = static_cast<int8_t>(bus.ReadDirect(addr + 2));
            inst.operandValue = static_cast<uint16_t>(addr + 3 + rel);
            break;
        }
    }
    return inst;
}

DisassembledInstruction Disassembler::Disassemble(const Bus& bus, uint16_t addr) {
    DecodedInstruction decoded = DecodeInstruction(bus, addr);

    DisassembledInstruction inst;
    inst.size = decoded.size;
    inst.operandValue = decoded.operandValue;
    inst.hasOperand = decoded.hasOperand;
    inst.mode = decoded.mode;

    std::stringstream ssAddr;
    ssAddr << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << addr;
    inst.address = ssAddr.str();

    if (decoded.isDataByte) {
        std::stringstream ssBytes;
        ssBytes << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(decoded.opcode) << " ";
        inst.bytes = ssBytes.str();
        inst.mnemonic = ".byte";
        std::stringstream ssOperands;
        ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(decoded.opcode);
        inst.operands = ssOperands.str();
        return inst;
    }

    const InstructionInfo& info = OpcodeTable.at(decoded.opcode);
    inst.mnemonic = info.mnemonic;

    std::stringstream ssBytes;
    ssBytes << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(decoded.opcode) << " ";

    std::stringstream ssOperands;
    switch (decoded.mode) {
        case AddressingMode::Implied:
            break;
        case AddressingMode::Accumulator:
            ssOperands << "A";
            break;
        case AddressingMode::Immediate: {
            uint8_t val = static_cast<uint8_t>(decoded.operandValue);
            ssBytes << std::setw(2) << static_cast<int>(val);
            ssOperands << "#$" << std::hex << std::uppercase << std::setw(2) << static_cast<int>(val);
            break;
        }
        case AddressingMode::ZeroPage: {
            uint8_t val = static_cast<uint8_t>(decoded.operandValue);
            ssBytes << std::setw(2) << static_cast<int>(val);
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << static_cast<int>(val);
            break;
        }
        case AddressingMode::ZeroPageX: {
            uint8_t val = static_cast<uint8_t>(decoded.operandValue);
            ssBytes << std::setw(2) << static_cast<int>(val);
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << static_cast<int>(val) << ",X";
            break;
        }
        case AddressingMode::ZeroPageY: {
            uint8_t val = static_cast<uint8_t>(decoded.operandValue);
            ssBytes << std::setw(2) << static_cast<int>(val);
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << static_cast<int>(val) << ",Y";
            break;
        }
        case AddressingMode::Absolute: {
            uint8_t low = static_cast<uint8_t>(decoded.operandValue & 0xFF);
            uint8_t high = static_cast<uint8_t>((decoded.operandValue >> 8) & 0xFF);
            ssBytes << std::setw(2) << static_cast<int>(low) << " " << std::setw(2) << static_cast<int>(high);
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << static_cast<int>(high) << std::setw(2) << static_cast<int>(low);
            break;
        }
        case AddressingMode::AbsoluteX: {
            uint8_t low = static_cast<uint8_t>(decoded.operandValue & 0xFF);
            uint8_t high = static_cast<uint8_t>((decoded.operandValue >> 8) & 0xFF);
            ssBytes << std::setw(2) << static_cast<int>(low) << " " << std::setw(2) << static_cast<int>(high);
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << static_cast<int>(high) << std::setw(2) << static_cast<int>(low)
                       << ",X";
            break;
        }
        case AddressingMode::AbsoluteY: {
            uint8_t low = static_cast<uint8_t>(decoded.operandValue & 0xFF);
            uint8_t high = static_cast<uint8_t>((decoded.operandValue >> 8) & 0xFF);
            ssBytes << std::setw(2) << static_cast<int>(low) << " " << std::setw(2) << static_cast<int>(high);
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << static_cast<int>(high) << std::setw(2) << static_cast<int>(low)
                       << ",Y";
            break;
        }
        case AddressingMode::Indirect: {
            uint8_t low = static_cast<uint8_t>(decoded.operandValue & 0xFF);
            uint8_t high = static_cast<uint8_t>((decoded.operandValue >> 8) & 0xFF);
            ssBytes << std::setw(2) << static_cast<int>(low) << " " << std::setw(2) << static_cast<int>(high);
            ssOperands << "($" << std::hex << std::uppercase << std::setw(2) << static_cast<int>(high) << std::setw(2) << static_cast<int>(low)
                       << ")";
            break;
        }
        case AddressingMode::IndirectX: {
            uint8_t val = static_cast<uint8_t>(decoded.operandValue);
            ssBytes << std::setw(2) << static_cast<int>(val);
            ssOperands << "($" << std::hex << std::uppercase << std::setw(2) << static_cast<int>(val) << ",X)";
            break;
        }
        case AddressingMode::IndirectY: {
            uint8_t val = static_cast<uint8_t>(decoded.operandValue);
            ssBytes << std::setw(2) << static_cast<int>(val);
            ssOperands << "($" << std::hex << std::uppercase << std::setw(2) << static_cast<int>(val) << "),Y";
            break;
        }
        case AddressingMode::Relative: {
            uint8_t offsetByte = bus.ReadDirect(addr + 1);
            ssBytes << std::setw(2) << static_cast<int>(offsetByte);
            ssOperands << "$" << std::hex << std::uppercase << std::setw(4) << decoded.operandValue;
            break;
        }
        case AddressingMode::ZeroPageIndirect: {
            uint8_t val = static_cast<uint8_t>(decoded.operandValue);
            ssBytes << std::setw(2) << static_cast<int>(val);
            ssOperands << "($" << std::hex << std::uppercase << std::setw(2) << static_cast<int>(val) << ")";
            break;
        }
        case AddressingMode::AbsoluteIndexedIndirect: {
            uint8_t low = static_cast<uint8_t>(decoded.operandValue & 0xFF);
            uint8_t high = static_cast<uint8_t>((decoded.operandValue >> 8) & 0xFF);
            ssBytes << std::setw(2) << static_cast<int>(low) << " " << std::setw(2) << static_cast<int>(high);
            ssOperands << "($" << std::hex << std::uppercase << std::setw(2) << static_cast<int>(high) << std::setw(2) << static_cast<int>(low)
                       << ",X)";
            break;
        }
        case AddressingMode::ZeroPageRelative: {
            uint8_t zpg = bus.ReadDirect(addr + 1);
            uint8_t rel = bus.ReadDirect(addr + 2);
            ssBytes << std::setw(2) << static_cast<int>(zpg) << " " << std::setw(2) << static_cast<int>(rel);
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << static_cast<int>(zpg) << ", $" << std::hex
                       << std::uppercase << std::setw(4) << decoded.operandValue;
            break;
        }
    }

    inst.bytes = ssBytes.str();
    inst.operands = ssOperands.str();

    return inst;
}

}  // namespace Hardware
