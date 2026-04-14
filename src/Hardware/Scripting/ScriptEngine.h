#pragma once

#include <atomic>
#include <deque>
#include <memory>
#include <string>

namespace Core {
class Emulator;
}

namespace Hardware {

class ScriptEngine {
   public:
    explicit ScriptEngine(Core::Emulator& emulator);
    ~ScriptEngine();

    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;
    ScriptEngine(ScriptEngine&&) = delete;
    ScriptEngine& operator=(ScriptEngine&&) = delete;

    bool LoadAndRun(const std::string& filepath);
    void Stop();
    [[nodiscard]] bool IsScriptRunning() const { return isRunning.load(); }

    [[nodiscard]] std::deque<std::string> GetOutput() const;
    void ClearOutput();
    void AppendOutput(const std::string& text);
    void SetMirrorToStdout(bool enabled) { mirrorToStdout = enabled; }
    [[nodiscard]] bool IsMirrorToStdoutEnabled() const { return mirrorToStdout; }

    Core::Emulator& GetEmulator() { return emulatorRef; }

    bool HasPendingScript() const;
    void ExecutePendingScript();

   private:
    Core::Emulator& emulatorRef;
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    
    std::atomic<bool> isRunning{false};
    std::atomic<bool> mirrorToStdout{false};
};

}  // namespace Hardware
