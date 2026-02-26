#include "Disassembler.h"

#include <iomanip>
#include <sstream>

#include "Hardware/CPU/Instructions/InstructionSet.h"

namespace Hardware {

DisassembledInstruction Disassembler::Disassemble(const Bus& bus, uint16_t addr) {
    DisassembledInstruction inst;
    inst.size = 1;

    std::stringstream ssAddr;
    ssAddr << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << addr;
    inst.address = ssAddr.str();

    uint8_t opcode = bus.ReadDirect(addr);
    const InstructionInfo& info = OpcodeTable.at(opcode);
    inst.mnemonic = info.mnemonic;

    std::stringstream ssBytes;
    ssBytes << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)opcode << " ";

    std::stringstream ssOperands;
    switch (info.mode) {
        case AddressingMode::Implied:
            inst.size = 1;
            break;
        case AddressingMode::Accumulator:
            ssOperands << "A";
            inst.size = 1;
            break;
        case AddressingMode::Immediate: {
            uint8_t val = bus.ReadDirect(addr + 1);
            ssBytes << std::setw(2) << (int)val;
            ssOperands << "#$" << std::hex << std::uppercase << std::setw(2) << (int)val;
            inst.size = 2;
            break;
        }
        case AddressingMode::ZeroPage: {
            uint8_t val = bus.ReadDirect(addr + 1);
            ssBytes << std::setw(2) << (int)val;
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << (int)val;
            inst.size = 2;
            break;
        }
        case AddressingMode::ZeroPageX: {
            uint8_t val = bus.ReadDirect(addr + 1);
            ssBytes << std::setw(2) << (int)val;
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << (int)val << ",X";
            inst.size = 2;
            break;
        }
        case AddressingMode::ZeroPageY: {
            uint8_t val = bus.ReadDirect(addr + 1);
            ssBytes << std::setw(2) << (int)val;
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << (int)val << ",Y";
            inst.size = 2;
            break;
        }
        case AddressingMode::Absolute: {
            uint8_t low = bus.ReadDirect(addr + 1);
            uint8_t high = bus.ReadDirect(addr + 2);
            ssBytes << std::setw(2) << (int)low << " " << std::setw(2) << (int)high;
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << (int)high << std::setw(2) << (int)low;
            inst.size = 3;
            break;
        }
        case AddressingMode::AbsoluteX: {
            uint8_t low = bus.ReadDirect(addr + 1);
            uint8_t high = bus.ReadDirect(addr + 2);
            ssBytes << std::setw(2) << (int)low << " " << std::setw(2) << (int)high;
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << (int)high << std::setw(2) << (int)low
                       << ",X";
            inst.size = 3;
            break;
        }
        case AddressingMode::AbsoluteY: {
            uint8_t low = bus.ReadDirect(addr + 1);
            uint8_t high = bus.ReadDirect(addr + 2);
            ssBytes << std::setw(2) << (int)low << " " << std::setw(2) << (int)high;
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << (int)high << std::setw(2) << (int)low
                       << ",Y";
            inst.size = 3;
            break;
        }
        case AddressingMode::Indirect: {
            uint8_t low = bus.ReadDirect(addr + 1);
            uint8_t high = bus.ReadDirect(addr + 2);
            ssBytes << std::setw(2) << (int)low << " " << std::setw(2) << (int)high;
            ssOperands << "($" << std::hex << std::uppercase << std::setw(2) << (int)high << std::setw(2) << (int)low
                       << ")";
            inst.size = 3;
            break;
        }
        case AddressingMode::IndirectX: {
            uint8_t val = bus.ReadDirect(addr + 1);
            ssBytes << std::setw(2) << (int)val;
            ssOperands << "($" << std::hex << std::uppercase << std::setw(2) << (int)val << ",X)";
            inst.size = 2;
            break;
        }
        case AddressingMode::IndirectY: {
            uint8_t val = bus.ReadDirect(addr + 1);
            ssBytes << std::setw(2) << (int)val;
            ssOperands << "($" << std::hex << std::uppercase << std::setw(2) << (int)val << "),Y";
            inst.size = 2;
            break;
        }
        case AddressingMode::Relative: {
            auto offset = (int8_t)bus.ReadDirect(addr + 1);
            uint16_t target = addr + 2 + offset;
            ssBytes << std::setw(2) << (int)(uint8_t)offset;
            ssOperands << "$" << std::hex << std::uppercase << std::setw(4) << target;
            inst.size = 2;
            break;
        }
        case AddressingMode::ZeroPageIndirect: {
            uint8_t val = bus.ReadDirect(addr + 1);
            ssBytes << std::setw(2) << (int)val;
            ssOperands << "($" << std::hex << std::uppercase << std::setw(2) << (int)val << ")";
            inst.size = 2;
            break;
        }
        case AddressingMode::AbsoluteIndexedIndirect: {
            uint8_t low = bus.ReadDirect(addr + 1);
            uint8_t high = bus.ReadDirect(addr + 2);
            ssBytes << std::setw(2) << (int)low << " " << std::setw(2) << (int)high;
            ssOperands << "($" << std::hex << std::uppercase << std::setw(2) << (int)high << std::setw(2) << (int)low
                       << ",X)";
            inst.size = 3;
            break;
        }
        case AddressingMode::ZeroPageRelative: {
            uint8_t zpg = bus.ReadDirect(addr + 1);
            auto rel = (int8_t)bus.ReadDirect(addr + 2);
            uint16_t target = addr + 3 + rel;
            ssBytes << std::setw(2) << (int)zpg << " " << std::setw(2) << (int)(uint8_t)rel;
            ssOperands << "$" << std::hex << std::uppercase << std::setw(2) << (int)zpg << ", $" << std::hex
                       << std::uppercase << std::setw(4) << target;
            inst.size = 3;
            break;
        }
    }

    inst.bytes = ssBytes.str();
    inst.operands = ssOperands.str();

    return inst;
}

}  // namespace Hardware
