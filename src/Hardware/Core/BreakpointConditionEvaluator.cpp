#include "Hardware/Core/BreakpointConditionEvaluator.h"

#include <array>
#include <iomanip>
#include <sstream>

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "Scripting/Lang/Interpreter.h"
#include "Scripting/Lang/Lexer.h"
#include "Scripting/Lang/Parser.h"

namespace Hardware {

BreakpointConditionEvaluator::BreakpointConditionEvaluator()
    : interpreter(std::make_unique<System::Hardware::Scripting::Lang::Interpreter>()) {}

BreakpointConditionEvaluator::~BreakpointConditionEvaluator() = default;

BreakpointConditionEvaluator::BreakpointConditionEvaluator(BreakpointConditionEvaluator&&) noexcept = default;
BreakpointConditionEvaluator& BreakpointConditionEvaluator::operator=(BreakpointConditionEvaluator&&) noexcept = default;

bool BreakpointConditionEvaluator::CheckBreakpoint(const Breakpoint& breakpoint, const CPU& cpu, const Bus& bus,
                                                   bool watchpointTriggered, uint16_t watchpointAddress) const {
    bool result = (breakpoint.compoundOp == LogicOp::And);
    for (const auto& cond : breakpoint.conditions) {
        bool condResult = EvaluateCondition(cond, cpu, bus, watchpointTriggered, watchpointAddress);
        if (breakpoint.compoundOp == LogicOp::And) {
            if (!condResult) {
                return false;
            }
        } else {
            if (condResult) {
                return true;
            }
        }
    }
    return result;
}

bool BreakpointConditionEvaluator::EvaluateCondition(const BreakCondition& cond, const CPU& cpu, const Bus& bus,
                                                     bool watchpointTriggered, uint16_t watchpointAddress) const {
    switch (cond.type) {
        case BreakpointType::PCAddress: {
            return cpu.PC == cond.address;
        }

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

        case BreakpointType::Expression: {
            if (!cond.ast) {
                return false;
            }
            if (!interpreter) {
                interpreter = std::make_unique<System::Hardware::Scripting::Lang::Interpreter>();
            }
            interpreter->setContext(&cpu, &bus);
            try {
                auto val = interpreter->evaluate(cond.ast.get());
                return System::Hardware::Scripting::Lang::Interpreter::isTruthy(val);
            } catch (...) {
                return false;
            }
        }
    }
    return false;
}

bool BreakpointConditionEvaluator::Compare(CompareOp compareOp, uint16_t lhs, uint16_t rhs) {
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

std::shared_ptr<System::Hardware::Scripting::Lang::Expr> BreakpointConditionEvaluator::ParseExpression(const std::string& expr) {
    if (expr.empty()) {
        return nullptr;
    }
    try {
        System::Hardware::Scripting::Lang::Lexer lexer(expr);
        auto tokens = lexer.scanTokens();
        System::Hardware::Scripting::Lang::Parser parser(tokens);
        return parser.parseExpression();
    } catch (...) {
        return nullptr;
    }
}

bool BreakpointConditionEvaluator::ValidateExpression(const std::string& expr, std::string* outError) {
    if (expr.empty()) {
        if (outError != nullptr) {
            *outError = "Expression is empty";
        }
        return false;
    }
    try {
        System::Hardware::Scripting::Lang::Lexer lexer(expr);
        auto tokens = lexer.scanTokens();
        for (const auto& token : tokens) {
            if (token.type == System::Hardware::Scripting::Lang::TokenType::Error) {
                if (outError != nullptr) {
                    *outError = token.lexeme;
                }
                return false;
            }
        }
        System::Hardware::Scripting::Lang::Parser parser(tokens);
        auto ast = parser.parseExpression();
        if (ast == nullptr) {
            if (outError != nullptr) {
                *outError = "Failed to parse expression";
            }
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        if (outError != nullptr) {
            *outError = e.what();
        }
        return false;
    } catch (...) {
        if (outError != nullptr) {
            *outError = "Unknown parse error";
        }
        return false;
    }
}

namespace {
constexpr std::array<const char*, 6> kOpNames = {"==", "!=", "<", "<=", ">", ">="};
constexpr std::array<const char*, 5> kRegNames = {"A", "X", "Y", "SP", "PC"};
constexpr std::array<const char*, 7> kFlagNames = {"C", "Z", "I", "D", "B", "V", "N"};
}  // namespace

std::string BreakpointConditionEvaluator::ConditionToString(const BreakCondition& cond) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0');
    switch (cond.type) {
        case BreakpointType::PCAddress:
            oss << "PC == 0x" << std::setw(4) << cond.address;
            break;
        case BreakpointType::RegisterCondition:
            oss << kRegNames.at(static_cast<size_t>(cond.reg)) << " " << kOpNames.at(static_cast<size_t>(cond.op)) << " 0x"
                << std::setw(cond.reg >= RegisterId::SP ? 4 : 2) << cond.value;
            break;
        case BreakpointType::FlagCondition:
            oss << kFlagNames.at(static_cast<size_t>(cond.flag)) << " " << kOpNames.at(static_cast<size_t>(cond.op)) << " "
                << cond.value;
            break;
        case BreakpointType::MemoryCondition:
            oss << "[0x" << std::setw(4) << cond.address << "] " << kOpNames.at(static_cast<size_t>(cond.op)) << " 0x"
                << std::setw(2) << cond.value;
            break;
        case BreakpointType::MemoryWatchpoint:
            oss << "write @ 0x" << std::setw(4) << cond.address;
            break;
        case BreakpointType::Expression:
            return cond.expression;
    }
    return oss.str();
}

std::string BreakpointConditionEvaluator::BreakpointSummary(const Breakpoint& bkpt) {
    std::string result;
    const char* joiner = (bkpt.compoundOp == LogicOp::And) ? " AND " : " OR ";
    for (size_t index = 0; index < bkpt.conditions.size(); index++) {
        if (index > 0) {
            result += joiner;
        }
        result += ConditionToString(bkpt.conditions[index]);
    }
    return result;
}

std::string BreakCondition::ToString() const {
    return BreakpointConditionEvaluator::ConditionToString(*this);
}

std::string Breakpoint::Summary() const {
    return BreakpointConditionEvaluator::BreakpointSummary(*this);
}

}  // namespace Hardware
