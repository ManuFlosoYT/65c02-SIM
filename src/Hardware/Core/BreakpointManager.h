#pragma once
#include <atomic>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "Hardware/Core/BreakpointConditionEvaluator.h"

namespace Hardware {

class CPU;
class Bus;

class BreakpointManager {
   public:
    BreakpointManager();
    ~BreakpointManager();
    BreakpointManager(const BreakpointManager&) = delete;
    BreakpointManager& operator=(const BreakpointManager&) = delete;
    BreakpointManager(BreakpointManager&&) = delete;
    BreakpointManager& operator=(BreakpointManager&&) = delete;

    uint32_t AddBreakpoint(Breakpoint breakpoint);
    void RemoveBreakpoint(uint32_t breakpointId);
    void SetEnabled(uint32_t breakpointId, bool enabled);
    void ClearAll();

    uint32_t Evaluate(const CPU& cpu, const Bus& bus);
    [[nodiscard]] bool HasWatchpointsFast() const { return hasWatchpoints.load(std::memory_order_relaxed); }

    [[nodiscard]] std::vector<Breakpoint> GetBreakpointsSnapshot() const;

    void NotifyWrite(uint16_t address, uint8_t value);
    void ClearWatchpointHit();

    [[nodiscard]] bool HasAnyBreakpointsFast() const { return hasAnyBreakpoints.load(std::memory_order_relaxed); }
    [[nodiscard]] bool HasComplexBreakpoints() const { return hasComplexBreakpoints.load(std::memory_order_relaxed); }
    [[nodiscard]] bool IsPCBreakpoint(uint16_t address) const { return fastPathBreakpoints.test(address); }

    static bool ValidateExpression(const std::string& expr, std::string* outError = nullptr) {
        return BreakpointConditionEvaluator::ValidateExpression(expr, outError);
    }

    [[nodiscard]] const BreakpointConditionEvaluator& GetConditionEvaluator() const { return conditionEvaluator; }

   private:
    void UpdateFastPath();

    mutable std::recursive_mutex bpMutex;
    std::vector<Breakpoint> breakpoints;
    uint32_t nextId = 1;

    std::atomic<bool> hasAnyBreakpoints{false};
    std::atomic<bool> hasComplexBreakpoints{false};
    std::atomic<bool> hasWatchpoints{false};
    std::bitset<65536> fastPathBreakpoints;
    std::vector<std::size_t> simplePCBreakpointIndices;
    std::vector<std::size_t> complexBreakpointIndices;

    bool watchpointTriggered = false;
    uint16_t watchpointAddress = 0;

    mutable BreakpointConditionEvaluator conditionEvaluator;

    [[nodiscard]] bool EvaluateCondition(const BreakCondition& cond, const CPU& cpu, const Bus& bus) const;
    [[nodiscard]] bool CheckBreakpoint(const Breakpoint& breakpoint, const CPU& cpu, const Bus& bus) const;
    static bool Compare(CompareOp compareOp, uint16_t lhs, uint16_t rhs);
};

}  // namespace Hardware
