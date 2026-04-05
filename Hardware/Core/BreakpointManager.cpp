#include "Hardware/Core/BreakpointManager.h"


#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware {

uint32_t BreakpointManager::AddBreakpoint(Breakpoint breakpoint) {
    std::lock_guard<std::recursive_mutex> lock(bpMutex);
    breakpoint.id = nextId++;
    breakpoints.push_back(std::move(breakpoint));
    UpdateFastPath();
    return breakpoints.back().id;
}

void BreakpointManager::RemoveBreakpoint(uint32_t breakpointId) {
    std::lock_guard<std::recursive_mutex> lock(bpMutex);
    std::erase_if(breakpoints, [breakpointId](const Breakpoint& entry) { return entry.id == breakpointId; });
    UpdateFastPath();
}

void BreakpointManager::SetEnabled(uint32_t breakpointId, bool enabled) {
    std::lock_guard<std::recursive_mutex> lock(bpMutex);
    for (auto& entry : breakpoints) {
        if (entry.id == breakpointId) {
            entry.enabled = enabled;
            break;
        }
    }
    UpdateFastPath();
}

void BreakpointManager::ClearAll() {
    std::lock_guard<std::recursive_mutex> lock(bpMutex);
    breakpoints.clear();
    watchpointTriggered = false;
    UpdateFastPath();
}

uint32_t BreakpointManager::Evaluate(const CPU& cpu, const Bus& bus) {
    if (!hasAnyBreakpoints.load(std::memory_order_relaxed)) {
        return 0;
    }
    std::lock_guard<std::recursive_mutex> lock(bpMutex);
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
                UpdateFastPath();
            }
            return entry.id;
        }
    }
    return 0;
}

bool BreakpointManager::HasActiveBreakpoints() const {
    return hasAnyBreakpoints.load(std::memory_order_relaxed);
}

bool BreakpointManager::HasWatchpoints() const {
    std::lock_guard<std::recursive_mutex> lock(bpMutex);
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
    std::lock_guard<std::recursive_mutex> lock(bpMutex);
    for (const auto& entry : breakpoints) {
        if (!entry.enabled) {
            continue;
        }
        for (const auto& cond : entry.conditions) {
            if (cond.type == BreakpointType::MemoryWatchpoint && cond.address == address) {
                watchpointAddress = address;
                watchpointTriggered = true;
                return;
            }
        }
    }
}

bool BreakpointManager::ConsumeWatchpointHit(uint16_t& hitAddress) {
    std::lock_guard<std::recursive_mutex> lock(bpMutex);
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

void BreakpointManager::UpdateFastPath() {
    fastPathBreakpoints.reset();
    bool any = false;
    for (const auto& breakpointEntry : breakpoints) {
        if (!breakpointEntry.enabled) {
            continue;
        }
        any = true;
        for (const auto& cond : breakpointEntry.conditions) {
            if (cond.type == BreakpointType::PCAddress) {
                fastPathBreakpoints.set(cond.address);
            }
        }
    }
    hasAnyBreakpoints.store(any, std::memory_order_relaxed);
}

}  // namespace Hardware
