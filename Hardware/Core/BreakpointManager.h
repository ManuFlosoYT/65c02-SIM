#pragma once
#include <atomic>
#include <bitset>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace Hardware {

class CPU;
class Bus;

enum class BreakpointType : uint8_t {
    PCAddress,
    RegisterCondition,
    FlagCondition,
    MemoryCondition,
    MemoryWatchpoint
};

enum class CompareOp : uint8_t { Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual };
enum class RegisterId : uint8_t { A, X, Y, SP, PC };
enum class FlagId : uint8_t { C, Z, I, D, B, V, N };
enum class LogicOp : uint8_t { And, Or };

struct BreakCondition {
    BreakpointType type = BreakpointType::PCAddress;
    RegisterId reg = RegisterId::A;
    FlagId flag = FlagId::Z;
    CompareOp op = CompareOp::Equal;
    uint16_t address = 0;
    uint16_t value = 0;
};

struct Breakpoint {
    uint32_t id = 0;
    std::string label;
    bool enabled = true;
    bool hitOnce = false;
    LogicOp compoundOp = LogicOp::And;
    std::vector<BreakCondition> conditions;
    uint64_t hitCount = 0;
};

class BreakpointManager {
   public:
    uint32_t AddBreakpoint(Breakpoint breakpoint);
    void RemoveBreakpoint(uint32_t breakpointId);
    void SetEnabled(uint32_t breakpointId, bool enabled);
    void ClearAll();

    uint32_t Evaluate(const CPU& cpu, const Bus& bus);
    [[nodiscard]] bool HasActiveBreakpoints() const;
    [[nodiscard]] bool HasWatchpoints() const;

    std::vector<Breakpoint>& GetBreakpoints();
    [[nodiscard]] const std::vector<Breakpoint>& GetBreakpoints() const;

    void NotifyWrite(uint16_t address, uint8_t value);
    bool ConsumeWatchpointHit(uint16_t& hitAddress);

    [[nodiscard]] bool HasAnyBreakpointsFast() const { return hasAnyBreakpoints.load(std::memory_order_relaxed); }
    [[nodiscard]] bool IsPCBreakpoint(uint16_t address) const { return fastPathBreakpoints.test(address); }

   private:
    void UpdateFastPath();

    mutable std::recursive_mutex bpMutex;
    std::vector<Breakpoint> breakpoints;
    uint32_t nextId = 1;

    std::atomic<bool> hasAnyBreakpoints{false};
    std::atomic<bool> hasComplexBreakpoints{false};
    std::bitset<65536> fastPathBreakpoints;

    bool watchpointTriggered = false;
    uint16_t watchpointAddress = 0;

    [[nodiscard]] bool EvaluateCondition(const BreakCondition& cond, const CPU& cpu, const Bus& bus) const;
    static bool Compare(CompareOp compareOp, uint16_t lhs, uint16_t rhs);
};

}  // namespace Hardware
