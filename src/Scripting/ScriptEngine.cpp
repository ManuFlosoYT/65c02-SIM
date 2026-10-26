#include "Scripting/ScriptEngine.h"

#include <fstream>
#include <iostream>
#include <deque>
#include <mutex>
#include <string>

#include "Hardware/Core/Emulator/Emulator.h"
#include "Scripting/EmulatorScriptBindings.h"
#include "Scripting/Lang/Lexer.h"
#include "Scripting/Lang/Parser.h"
#include "Scripting/Lang/Interpreter.h"

using namespace Hardware;
using namespace System::Hardware::Scripting::Lang;

struct ScriptEngine::Impl {
    ScriptEngine& parent;
    Core::Emulator& emulator;
    std::deque<std::string> outputQueue;
    std::mutex outputMutex;
    
    std::unique_ptr<Interpreter> interpreter;
    std::vector<std::unique_ptr<Stmt>> pendingStmts;
    std::atomic<bool> hasPending{false};
    std::mutex scriptMutex;
    std::atomic<bool> abortRequested{false};

    Impl(ScriptEngine& parentEngine, Core::Emulator& emu)
        : parent(parentEngine), emulator(emu), interpreter(std::make_unique<Interpreter>()) {
        interpreter->setContext(&emulator.GetCPU(), &emulator.GetMem());
        interpreter->setAbortCheck([this]() { return abortRequested.load(std::memory_order_relaxed); });
        interpreter->setPrintHandler([this](const std::string& text) {
            parent.AppendOutput(text);
        });
        EmulatorScriptBindings::Register(*interpreter, emulator, [this]() {
            return abortRequested.load(std::memory_order_relaxed);
        });
    }
};

ScriptEngine::ScriptEngine(Core::Emulator& emulator)
    : emulatorRef(emulator), pImpl(std::make_unique<Impl>(*this, emulator)) {}

ScriptEngine::~ScriptEngine() {
    Stop();
}

std::deque<std::string> ScriptEngine::GetOutput() const {
    std::lock_guard<std::mutex> lock(pImpl->outputMutex);
    return pImpl->outputQueue;
}

void ScriptEngine::ClearOutput() {
    std::lock_guard<std::mutex> lock(pImpl->outputMutex);
    pImpl->outputQueue.clear();
}

void ScriptEngine::AppendOutput(const std::string& text) {
    if (mirrorToStdout.load(std::memory_order_relaxed)) {
        std::cout << text << '\n';
    }
    std::lock_guard<std::mutex> lock(pImpl->outputMutex);
    pImpl->outputQueue.emplace_back(text);
}

bool ScriptEngine::LoadAndRun(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.good()) return false;
    
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    try {
        Lexer lexer(source);
        auto tokens = lexer.scanTokens();
        Parser parser(tokens);
        auto stmts = parser.parse();
        
        Stop();
        
        std::lock_guard<std::mutex> lock(pImpl->scriptMutex);
        pImpl->pendingStmts = std::move(stmts);
        pImpl->hasPending = true;
        pImpl->abortRequested = false;
        isRunning = true;
        
        // Wake up thread if asleep
        emulatorRef.Resume();
        return true;
    } catch(const std::exception& e) {
        AppendOutput(std::string("Script Parse Error: ") + e.what() + "\n");
        return false;
    }
}

void ScriptEngine::Stop() {
    if (pImpl) pImpl->abortRequested = true;
    isRunning = false;
}

bool ScriptEngine::HasPendingScript() const {
    return pImpl->hasPending.load(std::memory_order_relaxed);
}

void ScriptEngine::ExecutePendingScript() {
    std::vector<std::unique_ptr<Stmt>> stmts;
    {
        std::lock_guard<std::mutex> lock(pImpl->scriptMutex);
        if (!pImpl->hasPending) return;
        stmts = std::move(pImpl->pendingStmts);
        pImpl->hasPending = false;
    }
    
    try {
        pImpl->interpreter->interpret(stmts);
    } catch(const std::exception& e) {
        if (!pImpl->abortRequested.load(std::memory_order_relaxed)) {
            AppendOutput(std::string("Runtime Error: ") + e.what() + "\n");
        }
    }
    
    isRunning = false;
}
