#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace System::Hardware::Scripting::Lang {
struct Expr;
class Interpreter;
}

namespace Hardware {

class CPU;
class Bus;

enum class BreakpointType : uint8_t {
    PCAddress,
    RegisterCondition,
    FlagCondition,
    MemoryCondition,
    MemoryWatchpoint,
    Expression
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
    std::string expression;
    std::shared_ptr<System::Hardware::Scripting::Lang::Expr> ast;
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

class BreakpointConditionEvaluator {
   public:
    BreakpointConditionEvaluator();
    ~BreakpointConditionEvaluator();

    BreakpointConditionEvaluator(const BreakpointConditionEvaluator&) = delete;
    BreakpointConditionEvaluator& operator=(const BreakpointConditionEvaluator&) = delete;
    BreakpointConditionEvaluator(BreakpointConditionEvaluator&&) noexcept;
    BreakpointConditionEvaluator& operator=(BreakpointConditionEvaluator&&) noexcept;

    [[nodiscard]] bool EvaluateCondition(const BreakCondition& cond, const CPU& cpu, const Bus& bus,
                                         bool watchpointTriggered = false, uint16_t watchpointAddress = 0) const;
    [[nodiscard]] bool CheckBreakpoint(const Breakpoint& breakpoint, const CPU& cpu, const Bus& bus,
                                       bool watchpointTriggered = false, uint16_t watchpointAddress = 0) const;

    static bool Compare(CompareOp compareOp, uint16_t lhs, uint16_t rhs);
    static bool ValidateExpression(const std::string& expr, std::string* outError = nullptr);
    static std::shared_ptr<System::Hardware::Scripting::Lang::Expr> ParseExpression(const std::string& expr);

   private:
    mutable std::unique_ptr<System::Hardware::Scripting::Lang::Interpreter> interpreter;
};

}  // namespace Hardware
