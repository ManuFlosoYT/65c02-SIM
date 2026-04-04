#include "Hardware/Core/BreakpointManager.h"

#include <algorithm>

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware {

uint32_t BreakpointManager::AddBreakpoint(Breakpoint breakpoint) {
    breakpoint.id = nextId++;
    breakpoints.push_back(std::move(breakpoint));
    return breakpoints.back().id;
}

void BreakpointManager::RemoveBreakpoint(uint32_t breakpointId) {
    std::erase_if(breakpoints, [breakpointId](const Breakpoint& entry) { return entry.id == breakpointId; });
}

void BreakpointManager::SetEnabled(uint32_t breakpointId, bool enabled) {
    for (auto& entry : breakpoints) {
        if (entry.id == breakpointId) {
            entry.enabled = enabled;
            break;
        }
    }
}

void BreakpointManager::ClearAll() {
    breakpoints.clear();
    watchpointTriggered = false;
}

uint32_t BreakpointManager::Evaluate(const CPU& cpu, const Bus& bus) {
    for (auto& entry : breakpoints) {
        if (!entry.enabled || entry.conditions.empty()) {
            continue;
        }

        bool result = (entry.compoundOp == LogicOp::And);
        for (const auto& cond : entry.conditions) {
            bool condResult = EvaluateCondition(cond, cpu, bus);
            if (entry.compoundOp == LogicOp::And) {
                if (!condResult) {
                    result = false;
                    break;
                }
            } else {
                if (condResult) {
                    result = true;
                    break;
                }
            }
        }

        if (result) {
            entry.hitCount++;
            if (entry.hitOnce) {
                entry.enabled = false;
            }
            return entry.id;
        }
    }
    return 0;
}

bool BreakpointManager::HasActiveBreakpoints() const {
    return std::ranges::any_of(breakpoints, [](const Breakpoint& entry) { return entry.enabled; });
}

bool BreakpointManager::HasWatchpoints() const {
    for (const auto& entry : breakpoints) {
        if (!entry.enabled) {
            continue;
        }
        for (const auto& cond : entry.conditions) {
            if (cond.type == BreakpointType::MemoryWatchpoint) {
                return true;
            }
        }
    }
    return false;
}

std::vector<Breakpoint>& BreakpointManager::GetBreakpoints() { return breakpoints; }
const std::vector<Breakpoint>& BreakpointManager::GetBreakpoints() const { return breakpoints; }

void BreakpointManager::NotifyWrite(uint16_t address, uint8_t /*value*/) {
    for (const auto& entry : breakpoints) {
        if (!entry.enabled) {
            continue;
        }
        for (const auto& cond : entry.conditions) {
            if (cond.type == BreakpointType::MemoryWatchpoint && cond.address == address) {
                watchpointTriggered = true;
                watchpointAddress = address;
                return;
            }
        }
    }
}

bool BreakpointManager::ConsumeWatchpointHit(uint16_t& hitAddress) {
    if (watchpointTriggered) {
        hitAddress = watchpointAddress;
        watchpointTriggered = false;
        return true;
    }
    return false;
}

bool BreakpointManager::EvaluateCondition(const BreakCondition& cond, const CPU& cpu, const Bus& bus) const {
    switch (cond.type) {
        case BreakpointType::PCAddress:
            return cpu.PC == cond.address;

        case BreakpointType::RegisterCondition: {
            uint16_t regVal = 0;
            switch (cond.reg) {
                case RegisterId::A:
                    regVal = cpu.A;
                    break;
                case RegisterId::X:
                    regVal = cpu.X;
                    break;
                case RegisterId::Y:
                    regVal = cpu.Y;
                    break;
                case RegisterId::SP:
                    regVal = cpu.SP;
                    break;
                case RegisterId::PC:
                    regVal = cpu.PC;
                    break;
            }
            return Compare(cond.op, regVal, cond.value);
        }

        case BreakpointType::FlagCondition: {
            uint16_t flagVal = 0;
            switch (cond.flag) {
                case FlagId::C:
                    flagVal = cpu.C;
                    break;
                case FlagId::Z:
                    flagVal = cpu.Z;
                    break;
                case FlagId::I:
                    flagVal = cpu.I;
                    break;
                case FlagId::D:
                    flagVal = cpu.D;
                    break;
                case FlagId::B:
                    flagVal = cpu.B;
                    break;
                case FlagId::V:
                    flagVal = cpu.V;
                    break;
                case FlagId::N:
                    flagVal = cpu.N;
                    break;
            }
            return Compare(cond.op, flagVal, cond.value);
        }

        case BreakpointType::MemoryCondition: {
            uint16_t memVal = bus.ReadDirect(cond.address);
            return Compare(cond.op, memVal, cond.value);
        }

        case BreakpointType::MemoryWatchpoint:
            return watchpointTriggered && watchpointAddress == cond.address;
    }
    return false;
}

bool BreakpointManager::Compare(CompareOp compareOp, uint16_t lhs, uint16_t rhs) {
    switch (compareOp) {
        case CompareOp::Equal:
            return lhs == rhs;
        case CompareOp::NotEqual:
            return lhs != rhs;
        case CompareOp::Less:
            return lhs < rhs;
        case CompareOp::LessEqual:
            return lhs <= rhs;
        case CompareOp::Greater:
            return lhs > rhs;
        case CompareOp::GreaterEqual:
            return lhs >= rhs;
    }
    return false;
}

}  // namespace Hardware
