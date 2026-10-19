#include "Hardware/Core/BreakpointManager.h"

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Scripting/Lang/Interpreter.h"
#include "Hardware/Scripting/Lang/Lexer.h"
#include "Hardware/Scripting/Lang/Parser.h"

namespace Hardware {

BreakpointManager::BreakpointManager()
    : interpreter(std::make_unique<System::Hardware::Scripting::Lang::Interpreter>()) {}

BreakpointManager::~BreakpointManager() = default;

uint32_t BreakpointManager::AddBreakpoint(Breakpoint breakpoint) {
    std::scoped_lock lock(bpMutex);
    breakpoint.id = nextId++;
    for (auto& cond : breakpoint.conditions) {
        if (cond.type == BreakpointType::Expression && cond.ast == nullptr && !cond.expression.empty()) {
            try {
                System::Hardware::Scripting::Lang::Lexer lexer(cond.expression);
                auto tokens = lexer.scanTokens();
                System::Hardware::Scripting::Lang::Parser parser(tokens);
                cond.ast = parser.parseExpression();
            } catch (...) {
                cond.ast = nullptr;
            }
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

    // Lock-free check for the most common case: No PC breakpoint at this address
    // and no complex breakpoints (registers, memory, etc.) active.
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
            if (CheckBreakpoint(entry, cpu, bus)) {
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
    bool result = (breakpoint.compoundOp == LogicOp::And);
    for (const auto& cond : breakpoint.conditions) {
        bool condResult = EvaluateCondition(cond, cpu, bus);
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

bool BreakpointManager::ValidateExpression(const std::string& expr, std::string* outError) {
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

}  // namespace Hardware
