#include "Hardware/Core/BreakpointManager.h"

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware {

BreakpointManager::BreakpointManager() = default;
BreakpointManager::~BreakpointManager() = default;

uint32_t BreakpointManager::AddBreakpoint(Breakpoint breakpoint) {
    std::scoped_lock lock(bpMutex);
    breakpoint.id = nextId++;
    for (auto& cond : breakpoint.conditions) {
        if (cond.type == BreakpointType::Expression && cond.ast == nullptr && !cond.expression.empty()) {
            cond.ast = BreakpointConditionEvaluator::ParseExpression(cond.expression);
        }
    }
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

    const bool pcCandidate = fastPathBreakpoints.test(cpu.PC);
    const bool hasComplex = hasComplexBreakpoints.load(std::memory_order_relaxed);

    if (!pcCandidate && !hasComplex) {
        return 0;
    }

    std::lock_guard<std::recursive_mutex> lock(bpMutex);
    if (pcCandidate) {
        for (const std::size_t idx : simplePCBreakpointIndices) {
            auto& entry = breakpoints[idx];
            if (!entry.enabled || entry.conditions.empty()) {
                continue;
            }
            if (entry.conditions.front().address != cpu.PC) {
                continue;
            }
            entry.hitCount++;
            if (entry.hitOnce) {
                entry.enabled = false;
                UpdateFastPath();
            }
            watchpointTriggered = false;
            return entry.id;
        }
    }

    if (hasComplex) {
        for (const std::size_t idx : complexBreakpointIndices) {
            auto& entry = breakpoints[idx];
            if (!entry.enabled || entry.conditions.empty()) {
                continue;
            }
            if (conditionEvaluator.CheckBreakpoint(entry, cpu, bus, watchpointTriggered, watchpointAddress)) {
                entry.hitCount++;
                if (entry.hitOnce) {
                    entry.enabled = false;
                    UpdateFastPath();
                }
                watchpointTriggered = false;
                return entry.id;
            }
        }
    }

    watchpointTriggered = false;
    return 0;
}

bool BreakpointManager::CheckBreakpoint(const Breakpoint& breakpoint, const CPU& cpu, const Bus& bus) const {
    return conditionEvaluator.CheckBreakpoint(breakpoint, cpu, bus, watchpointTriggered, watchpointAddress);
}

std::vector<Breakpoint> BreakpointManager::GetBreakpointsSnapshot() const {
    std::lock_guard<std::recursive_mutex> lock(bpMutex);
    return breakpoints;
}

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

void BreakpointManager::ClearWatchpointHit() {
    std::lock_guard<std::recursive_mutex> lock(bpMutex);
    watchpointTriggered = false;
}

bool BreakpointManager::EvaluateCondition(const BreakCondition& cond, const CPU& cpu, const Bus& bus) const {
    return conditionEvaluator.EvaluateCondition(cond, cpu, bus, watchpointTriggered, watchpointAddress);
}

bool BreakpointManager::Compare(CompareOp compareOp, uint16_t lhs, uint16_t rhs) {
    return BreakpointConditionEvaluator::Compare(compareOp, lhs, rhs);
}

void BreakpointManager::UpdateFastPath() {
    fastPathBreakpoints.reset();
    simplePCBreakpointIndices.clear();
    complexBreakpointIndices.clear();
    bool any = false;
    bool complex = false;
    bool watchpoints = false;
    for (std::size_t i = 0; i < breakpoints.size(); ++i) {
        const auto& breakpointEntry = breakpoints[i];
        if (!breakpointEntry.enabled) {
            continue;
        }
        any = true;
        const bool isSimplePCOnly =
            breakpointEntry.conditions.size() == 1 &&
            breakpointEntry.conditions.front().type == BreakpointType::PCAddress &&
            breakpointEntry.compoundOp == LogicOp::And;

        if (isSimplePCOnly) {
            fastPathBreakpoints.set(breakpointEntry.conditions.front().address);
            simplePCBreakpointIndices.push_back(i);
            continue;
        }

        complexBreakpointIndices.push_back(i);
        complex = true;
        for (const auto& cond : breakpointEntry.conditions) {
            if (cond.type == BreakpointType::PCAddress) {
                fastPathBreakpoints.set(cond.address);
            }
            if (cond.type == BreakpointType::MemoryWatchpoint) {
                watchpoints = true;
            }
            if (cond.type == BreakpointType::Expression) {
                if (cond.expression.find("MEM") != std::string::npos ||
                    cond.expression.find("mem") != std::string::npos ||
                    cond.expression.find("changed") != std::string::npos) {
                    watchpoints = true;
                }
            }
        }
    }
    hasAnyBreakpoints.store(any, std::memory_order_relaxed);
    hasComplexBreakpoints.store(complex, std::memory_order_relaxed);
    hasWatchpoints.store(watchpoints, std::memory_order_relaxed);
}

}  // namespace Hardware
