#include "Hardware/Core/SubroutineProfiler.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <unordered_map>
#include <utility>

#include "Hardware/Core/SymbolTable.h"

namespace Hardware {

std::shared_ptr<FlameGraphNode> FlameGraphNode::FindOrAddChild(uint16_t childAddr, const std::string& childName,
                                                               bool interrupt) {
    for (const auto& child : children) {
        if (child && child->address == childAddr && child->isInterrupt == interrupt) {
            if (!childName.empty() && (child->name.empty() || (child->name[0] == '$' && childName[0] != '$'))) {
                child->name = childName;
            }
            return child;
        }
    }
    auto newChild = std::make_shared<FlameGraphNode>();
    newChild->address = childAddr;
    newChild->name = childName;
    newChild->isInterrupt = interrupt;
    children.push_back(newChild);
    return newChild;
}

SubroutineProfiler::SubroutineProfiler() {
    root = std::make_shared<FlameGraphNode>();
    root->name = "Root";
}

void SubroutineProfiler::SetEnabled(bool isEnabled) {
    std::lock_guard<std::mutex> lock(profilerMutex);
    enabled.store(isEnabled, std::memory_order_release);
}

bool SubroutineProfiler::IsEnabled() const { return enabled.load(std::memory_order_acquire); }

void SubroutineProfiler::Clear() {
    std::lock_guard<std::mutex> lock(profilerMutex);
    root = std::make_shared<FlameGraphNode>();
    root->name = "Root";
    callStack.clear();
    pageCrossTracker.ClearAlerts();
    lastCycle = 0;
}

void SubroutineProfiler::Reset() { Clear(); }

void SubroutineProfiler::ClearPageCrossAlerts() {
    std::lock_guard<std::mutex> lock(profilerMutex);
    pageCrossTracker.ClearAlerts();
}

void SubroutineProfiler::SetSymbolTable(const Control::SymbolTable* symTable) {
    std::lock_guard<std::mutex> lock(profilerMutex);
    symbolTable = symTable;
    if (symbolTable != nullptr && root != nullptr) {
        auto updateNames = [&](auto& self, const std::shared_ptr<FlameGraphNode>& node) -> void {
            if (!node) {
                return;
            }
            if (node->address != 0 && (node->name.empty() || node->name[0] == '$')) {
                std::string resolved = ResolveName(node->address);
                if (!resolved.empty() && resolved[0] != '$') {
                    node->name = std::move(resolved);
                }
            }
            for (const auto& child : node->children) {
                self(self, child);
            }
        };
        updateNames(updateNames, root);
        for (auto& alert : pageCrossTracker.GetAlerts()) {
            if (alert.subroutineAddr != 0 && (alert.subroutineName.empty() || alert.subroutineName[0] == '$')) {
                std::string resolved = ResolveName(alert.subroutineAddr);
                if (!resolved.empty() && resolved[0] != '$') {
                    alert.subroutineName = std::move(resolved);
                }
            }
        }
    }
}

void SubroutineProfiler::OnJSR(uint16_t targetAddr, uint16_t returnAddr, uint64_t currentCycle) {
    if (!enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(profilerMutex);
    if (!enabled.load(std::memory_order_relaxed)) {
        return;
    }
    lastCycle = std::max(lastCycle, currentCycle);

    std::string name = ResolveName(targetAddr);
    auto parent = callStack.empty() ? root : callStack.back().node;
    auto node = parent->FindOrAddChild(targetAddr, name, false);
    node->callCount++;
    callStack.push_back({node, currentCycle, 0, returnAddr, false});
}

void SubroutineProfiler::OnRTS(uint64_t currentCycle) {
    if (!enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(profilerMutex);
    if (!enabled.load(std::memory_order_relaxed)) {
        return;
    }
    lastCycle = std::max(lastCycle, currentCycle);
    PopFrame(currentCycle);
}

void SubroutineProfiler::OnBRK(uint16_t vectorAddr, uint16_t pc, uint64_t currentCycle) {
    if (!enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(profilerMutex);
    if (!enabled.load(std::memory_order_relaxed)) {
        return;
    }
    lastCycle = std::max(lastCycle, currentCycle);

    std::string name = ResolveName(vectorAddr);
    auto parent = callStack.empty() ? root : callStack.back().node;
    auto node = parent->FindOrAddChild(vectorAddr, name, true);
    node->callCount++;
    callStack.push_back({node, currentCycle, 0, pc, true});
}

void SubroutineProfiler::OnRTI(uint64_t currentCycle) {
    if (!enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(profilerMutex);
    if (!enabled.load(std::memory_order_relaxed)) {
        return;
    }
    lastCycle = std::max(lastCycle, currentCycle);
    PopFrame(currentCycle);
}

void SubroutineProfiler::OnInterrupt(uint16_t vectorAddr, uint64_t currentCycle, const std::string& name) {
    if (!enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(profilerMutex);
    if (!enabled.load(std::memory_order_relaxed)) {
        return;
    }
    lastCycle = std::max(lastCycle, currentCycle);

    std::string handlerName = name.empty() ? ResolveName(vectorAddr) : name;
    auto parent = callStack.empty() ? root : callStack.back().node;
    auto node = parent->FindOrAddChild(vectorAddr, handlerName, true);
    node->callCount++;
    callStack.push_back({node, currentCycle, 0, 0, true});
}

void SubroutineProfiler::OnPageCross(uint16_t pc, uint16_t baseAddr, uint16_t effectiveAddr) {
    if (!enabled.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(profilerMutex);
    if (!enabled.load(std::memory_order_relaxed)) {
        return;
    }

    uint16_t subAddr = 0;
    std::string subName = "Root";
    if (!callStack.empty()) {
        auto& activeNode = callStack.back().node;
        activeNode->pageCrossPenalties++;
        subAddr = activeNode->address;
        subName = activeNode->name;
    } else {
        root->pageCrossPenalties++;
    }

    pageCrossTracker.OnPageCross(pc, baseAddr, effectiveAddr, subAddr, subName);
}

void SubroutineProfiler::PopFrame(uint64_t currentCycle) {
    if (callStack.empty()) {
        return;
    }
    CallFrame frame = callStack.back();
    callStack.pop_back();

    uint64_t duration = (currentCycle >= frame.startCycle) ? (currentCycle - frame.startCycle) : 0;
    frame.node->totalCycles += duration;
    uint64_t childDuration = std::min(frame.childCycles, duration);
    frame.node->selfCycles += (duration - childDuration);

    if (!callStack.empty()) {
        callStack.back().childCycles += duration;
    }
}

SubroutineProfiler::Snapshot SubroutineProfiler::GetSnapshot() const {
    std::lock_guard<std::mutex> lock(profilerMutex);

    Snapshot snapshot;
    if (!root) {
        snapshot.root = std::make_shared<FlameGraphNode>();
        snapshot.root->name = "Root";
        return snapshot;
    }

    std::unordered_map<const FlameGraphNode*, std::shared_ptr<FlameGraphNode>> nodeMap;
    auto cloneTree = [&](auto& self, const std::shared_ptr<FlameGraphNode>& src) -> std::shared_ptr<FlameGraphNode> {
        if (!src) {
            return nullptr;
        }
        auto dst = std::make_shared<FlameGraphNode>();
        dst->address = src->address;
        dst->name = src->name;
        if (dst->address != 0 && (dst->name.empty() || dst->name[0] == '$') && symbolTable != nullptr) {
            std::string resolved = ResolveName(dst->address);
            if (!resolved.empty() && resolved[0] != '$') {
                dst->name = std::move(resolved);
            }
        }
        dst->totalCycles = src->totalCycles;
        dst->selfCycles = src->selfCycles;
        dst->callCount = src->callCount;
        dst->pageCrossPenalties = src->pageCrossPenalties;
        dst->isInterrupt = src->isInterrupt;
        nodeMap[src.get()] = dst;

        dst->children.reserve(src->children.size());
        for (const auto& child : src->children) {
            auto childClone = self(self, child);
            if (childClone) {
                dst->children.push_back(std::move(childClone));
            }
        }
        return dst;
    };

    snapshot.root = cloneTree(cloneTree, root);

    uint64_t currentCycle = lastCycle;
    uint64_t activeChildElapsed = 0;
    for (auto it = callStack.rbegin(); it != callStack.rend(); ++it) {
        const auto& frame = *it;
        auto mapIt = nodeMap.find(frame.node.get());
        if (mapIt != nodeMap.end() && mapIt->second && currentCycle >= frame.startCycle) {
            uint64_t elapsed = currentCycle - frame.startCycle;
            uint64_t childDuration = frame.childCycles + activeChildElapsed;
            uint64_t selfElapsed = (elapsed > childDuration) ? (elapsed - childDuration) : 0;
            mapIt->second->totalCycles += elapsed;
            mapIt->second->selfCycles += selfElapsed;
            activeChildElapsed = elapsed;
        }
    }

    uint64_t sumChildren = 0;
    for (const auto& child : snapshot.root->children) {
        if (child) {
            sumChildren += child->totalCycles;
        }
    }
    snapshot.root->totalCycles = sumChildren;
    snapshot.totalCycles = sumChildren;

    snapshot.pageCrossAlerts = pageCrossTracker.GetAlerts();
    if (symbolTable != nullptr) {
        for (auto& alert : snapshot.pageCrossAlerts) {
            if (alert.subroutineAddr != 0 && (alert.subroutineName.empty() || alert.subroutineName[0] == '$')) {
                std::string resolved = ResolveName(alert.subroutineAddr);
                if (!resolved.empty() && resolved[0] != '$') {
                    alert.subroutineName = std::move(resolved);
                }
            }
        }
    }
    return snapshot;
}

std::string SubroutineProfiler::ResolveName(uint16_t addr) const {
    if (symbolTable != nullptr) {
        const auto* sym = symbolTable->GetSymbolAtAddress(addr);
        if (sym != nullptr && !sym->name.empty()) {
            return sym->name;
        }
    }
    std::array<char, 6> buf{};
    std::snprintf(buf.data(), buf.size(), "$%04X", addr);
    return std::string(buf.data());
}

}  // namespace Hardware
