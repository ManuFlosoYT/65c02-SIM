#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Control {
class SymbolTable;
}

namespace Hardware {

struct FlameGraphNode {
    uint16_t address = 0;
    std::string name;
    uint64_t totalCycles = 0;
    uint64_t selfCycles = 0;
    uint32_t callCount = 0;
    uint32_t pageCrossPenalties = 0;
    bool isInterrupt = false;
    std::vector<std::shared_ptr<FlameGraphNode>> children;

    std::shared_ptr<FlameGraphNode> FindOrAddChild(uint16_t childAddr, const std::string& childName, bool interrupt);
};

struct PageCrossAlert {
    uint16_t pc = 0;
    uint16_t baseAddr = 0;
    uint16_t effectiveAddr = 0;
    uint16_t subroutineAddr = 0;
    std::string subroutineName;
    uint32_t count = 0;
};

class SubroutineProfiler {
   public:
    struct Snapshot {
        std::shared_ptr<FlameGraphNode> root;
        uint64_t totalCycles = 0;
        std::vector<PageCrossAlert> pageCrossAlerts;
    };

    SubroutineProfiler();
    ~SubroutineProfiler() = default;

    SubroutineProfiler(const SubroutineProfiler&) = delete;
    SubroutineProfiler& operator=(const SubroutineProfiler&) = delete;
    SubroutineProfiler(SubroutineProfiler&&) = delete;
    SubroutineProfiler& operator=(SubroutineProfiler&&) = delete;

    void SetEnabled(bool enabled);
    [[nodiscard]] bool IsEnabled() const;

    void Clear();
    void Reset();

    void SetSymbolTable(const Control::SymbolTable* symTable);

    void OnJSR(uint16_t targetAddr, uint16_t returnAddr, uint64_t currentCycle);
    void OnRTS(uint64_t currentCycle);
    void OnBRK(uint16_t vectorAddr, uint16_t pc, uint64_t currentCycle);
    void OnRTI(uint64_t currentCycle);
    void OnInterrupt(uint16_t vectorAddr, uint64_t currentCycle, const std::string& name);
    void OnPageCross(uint16_t pc, uint16_t baseAddr, uint16_t effectiveAddr);

    [[nodiscard]] Snapshot GetSnapshot() const;

   private:
    struct CallFrame {
        std::shared_ptr<FlameGraphNode> node;
        uint64_t startCycle = 0;
        uint64_t childCycles = 0;
        uint16_t returnAddr = 0;
        bool isInterrupt = false;
    };

    void PopFrame(uint64_t currentCycle);
    [[nodiscard]] std::string ResolveName(uint16_t addr) const;

    mutable std::mutex profilerMutex;
    std::atomic<bool> enabled{false};
    const Control::SymbolTable* symbolTable = nullptr;

    std::shared_ptr<FlameGraphNode> root;
    std::vector<CallFrame> callStack;
    std::vector<PageCrossAlert> pageCrossAlerts;
    uint64_t lastCycle = 0;
};

}  // namespace Hardware
