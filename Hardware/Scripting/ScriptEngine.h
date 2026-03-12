#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <deque>

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

    [[nodiscard]] std::deque<std::string> GetOutput() const;
    void ClearOutput();
    void AppendOutput(const std::string& text);

    Core::Emulator& GetEmulator() { return emulatorRef; }

   private:
    void ScriptThread(const std::string& filepath);

    Core::Emulator& emulatorRef;
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    std::thread workerThread;
    std::atomic<bool> isRunning{false};
};

}  // namespace Hardware
