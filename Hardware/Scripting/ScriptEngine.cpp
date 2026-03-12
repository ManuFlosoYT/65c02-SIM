#include "Hardware/Scripting/ScriptEngine.h"

#include <pocketpy.h>

#include <fstream>
#include <iostream>
#include <deque>
#include <mutex>

#include "Hardware/Core/Emulator.h"

using namespace Hardware;

struct ScriptEngine::Impl {
    Core::Emulator& emulator;
    std::deque<std::string> outputQueue;
    std::mutex outputMutex;

    explicit Impl(Core::Emulator& emu) : emulator(emu) {}
    ~Impl() = default;
    
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;
};

ScriptEngine::ScriptEngine(Core::Emulator& emulator)
    : emulatorRef(emulator), pImpl(std::make_unique<Impl>(emulator)) {}

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
    std::lock_guard<std::mutex> lock(pImpl->outputMutex);
    pImpl->outputQueue.emplace_back(text);
}

bool ScriptEngine::LoadAndRun(const std::string& filepath) {
    Stop();
    isRunning = true;
    workerThread = std::thread(&ScriptEngine::ScriptThread, this, filepath);
    return true;
}

void ScriptEngine::Stop() {
    isRunning = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

static bool py_emu_pause(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    engine->GetEmulator().Pause();
    py_newnone(py_retval());
    return true;
}

static bool py_emu_resume(int argc, py_StackRef argv) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    engine->GetEmulator().Resume();
    py_newnone(py_retval());
    return true;
}

static void py_custom_print(const char* text) {
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    if (engine != nullptr) {
        engine->AppendOutput(text);
    } else {
        std::cout << text << std::flush;
    }
}

static bool py_emu_read_mem(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);           // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_int);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    auto addr = static_cast<uint16_t>(py_toint(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    uint8_t val = engine->GetEmulator().GetMem().Read(addr);
    py_newint(py_retval(), val);
    return true;
}

static bool py_emu_write_mem(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(2);           // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_int);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    PY_CHECK_ARG_TYPE(1, tp_int);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    auto addr = static_cast<uint16_t>(py_toint(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto val = static_cast<uint8_t>(py_toint(py_arg(1)));    // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    engine->GetEmulator().GetMem().Write(addr, val);
    py_newnone(py_retval());
    return true;
}

static bool py_emu_wait_cycles(int argc, py_StackRef argv) {
    PY_CHECK_ARGC(1);           // NOLINT(cppcoreguidelines-pro-type-vararg)
    PY_CHECK_ARG_TYPE(0, tp_int);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* engine = static_cast<ScriptEngine*>(py_getvmctx());
    auto cycles = static_cast<int>(py_toint(py_arg(0)));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)

    if (engine->GetEmulator().IsPaused()) {
        for (int i = 0; i < cycles; i++) {
            engine->GetEmulator().Step();
        }
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(cycles));
    }
    
    py_newnone(py_retval());
    return true;
}

void ScriptEngine::ScriptThread(const std::string& filepath) {
    py_initialize();
    py_setvmctx(this);
    py_callbacks()->print = py_custom_print;
    
    py_GlobalRef mod = py_newmodule("emu");
    py_bindfunc(mod, "pause", py_emu_pause);
    py_bindfunc(mod, "resume", py_emu_resume);
    py_bindfunc(mod, "read_mem", py_emu_read_mem);
    py_bindfunc(mod, "write_mem", py_emu_write_mem);
    py_bindfunc(mod, "wait_cycles", py_emu_wait_cycles);

    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "ScriptEngine: Failed to open script " << filepath << '\n';
        isRunning = false;
        py_finalize();
        return;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (!py_exec(content.c_str(), filepath.c_str(), EXEC_MODE, nullptr)) {
        py_printexc();
    }
    
    py_finalize();
    isRunning = false;
}
