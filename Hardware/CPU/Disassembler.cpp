#include "Disassembler.h"

#include <iomanip>
#include <sstream>

#include "Instructions/InstructionSet.h"

namespace Hardware {

DisassembledInstruction Disassembler::Disassemble(const Mem& mem, uint16_t addr) {
    DisassembledInstruction di;
    di.size = 1;

    std::stringstream ssAddr;
    ssAddr << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << addr;
    di.address = ssAddr.str();

    uint8_t opcode = mem.Peek(addr);
    const InstructionInfo& info = OpcodeTable[opcode];
    di.mnemonic = info.mnemonic;

    std::stringstream ssBytes;
    ssBytes << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)opcode << " ";

    std::stringstream ssOperands;
    switch (info.mode) {
        case AddressingMode::Implied:
            di.size = 1;
            break;
        case AddressingMode::Accumulator:
            ssOperands << "A";
            di.size = 1;
            break;
        case AddressingMode::Immediate: {
            uint8_t val = mem.Peek(addr + 1);
            ssBytes << std::setw(2) << (int)val;
            ssOperands << "#$" << std::hex << std::uppercase << std::setw(2) << (int)val;
            di.size = 2;
            break;
        }
        case AddressingMode::ZeroPage: {
            uint8_t val = mem.Peek(addr + 1);
            ssBytes << std::setw(2) << (int)val;
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << (int)val;
            di.size = 2;
            break;
        }
        case AddressingMode::ZeroPageX: {
            uint8_t val = mem.Peek(addr + 1);
            ssBytes << std::setw(2) << (int)val;
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << (int)val << ",X";
            di.size = 2;
            break;
        }
        case AddressingMode::ZeroPageY: {
            uint8_t val = mem.Peek(addr + 1);
            ssBytes << std::setw(2) << (int)val;
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << (int)val << ",Y";
            di.size = 2;
            break;
        }
        case AddressingMode::Absolute: {
            uint8_t low = mem.Peek(addr + 1);
            uint8_t high = mem.Peek(addr + 2);
            ssBytes << std::setw(2) << (int)low << " " << std::setw(2) << (int)high;
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << (int)high 
                << std::setw(2) << (int)low;
            di.size = 3;
            break;
        }
        case AddressingMode::AbsoluteX: {
            uint8_t low = mem.Peek(addr + 1);
            uint8_t high = mem.Peek(addr + 2);
            ssBytes << std::setw(2) << (int)low << " " << std::setw(2) << (int)high;
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << (int)high 
                << std::setw(2) << (int)low << ",X";
            di.size = 3;
            break;
        }
        case AddressingMode::AbsoluteY: {
            uint8_t low = mem.Peek(addr + 1);
            uint8_t high = mem.Peek(addr + 2);
            ssBytes << std::setw(2) << (int)low << " " << std::setw(2) << (int)high;   
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2)
                       << (int)high << std::setw(2) << (int)low << ",Y";
            di.size = 3;
            break;
        }
        case AddressingMode::Indirect: {
            uint8_t low = mem.Peek(addr + 1);
            uint8_t high = mem.Peek(addr + 2);
            ssBytes << std::setw(2) << (int)low << " " << std::setw(2)
                    << (int)high;
            ssOperands << "($" << std::hex << std::uppercase << std::setw(2)
                       << (int)high << std::setw(2) << (int)low << ")";
            di.size = 3;
            break;
        }
        case AddressingMode::IndirectX: {
            uint8_t val = mem.Peek(addr + 1);
            ssBytes << std::setw(2) << (int)val;
            ssOperands << "($" << std::hex << std::uppercase << std::setw(2)
                       << (int)val << ",X)";
            di.size = 2;
            break;
        }
        case AddressingMode::IndirectY: {
            uint8_t val = mem.Peek(addr + 1);
            ssBytes << std::setw(2) << (int)val;
            ssOperands << "($" << std::hex << std::uppercase << std::setw(2)
                       << (int)val << "),Y";
            di.size = 2;
            break;
        }
        case AddressingMode::Relative: {
            int8_t offset = (int8_t)mem.Peek(addr + 1);
            uint16_t target = addr + 2 + offset;
            ssBytes << std::setw(2) << (int)(uint8_t)offset;
            ssOperands << "$" << std::hex << std::uppercase << std::setw(4)
                       << target;
            di.size = 2;
            break;
        }
        case AddressingMode::ZeroPageIndirect: {
            uint8_t val = mem.Peek(addr + 1);
            ssBytes << std::setw(2) << (int)val;
            ssOperands << "($" << std::hex << std::uppercase << std::setw(2)
                       << (int)val << ")";
            di.size = 2;
            break;
        }
        case AddressingMode::AbsoluteIndexedIndirect: {
            uint8_t low = mem.Peek(addr + 1);
            uint8_t high = mem.Peek(addr + 2);
            ssBytes << std::setw(2) << (int)low << " " << std::setw(2)
                    << (int)high;
            ssOperands << "($" << std::hex << std::uppercase << std::setw(2)
                       << (int)high << std::setw(2) << (int)low << ",X)";
            di.size = 3;
            break;
        }
        case AddressingMode::ZeroPageRelative: {
            uint8_t zp = mem.Peek(addr + 1);
            int8_t rel = (int8_t)mem.Peek(addr + 2);
            uint16_t target = addr + 3 + rel;
            ssBytes << std::setw(2) << (int)zp << " " << std::setw(2)
                    << (int)(uint8_t)rel;
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2)
                       << (int)zp << ", $" << std::hex << std::uppercase
                       << std::setw(4) << target;
            di.size = 3;
            break;
        }
    }

    di.bytes = ssBytes.str();
    di.operands = ssOperands.str();

    return di;
}

}  // namespace Hardware
