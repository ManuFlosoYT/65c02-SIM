#include "Frontend/Platform/PlatformUtils.h"

#include <SDL3/SDL.h>
#include <csetjmp>
#include <csignal>
#include <execinfo.h>
#include <pthread.h>
#include <unistd.h>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <string>
#include <system_error>

namespace Frontend::Platform {

bool OpenURL(std::string_view url) {
    if (url.empty()) {
        return false;
    }
    std::string nullTerminatedUrl(url);
    return SDL_OpenURL(nullTerminatedUrl.c_str());
}

std::filesystem::path GetExecutableDirectory() {
    std::error_code ec;
    auto exePath = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec && !exePath.empty()) {
        return exePath.parent_path();
    }
    return std::filesystem::current_path();
}

char GetEnvPathDelimiter() {
    return ':';
}

std::string AppendExecutableExtension(std::string_view binaryName) {
    return std::string(binaryName);
}

std::string CaptureStackTrace() {
    std::array<void*, 64> callstack{};
    int frames = backtrace(callstack.data(), static_cast<int>(callstack.size()));
    char** strs = backtrace_symbols(callstack.data(), frames);
    if (!strs) {
        return "No stack trace available.";
    }
    std::string result;
    for (int i = 0; i < frames; ++i) {
        result += strs[i];
        result += '\n';
    }
    free(strs);
    return result;
}

struct PosixCrashContext {
    sigjmp_buf jumpBuf;
    PosixCrashContext* prev = nullptr;
    std::string* outErrorMessage = nullptr;
};

static thread_local PosixCrashContext* t_posixCrashContext = nullptr;

static void CrashSignalHandler(int sig, siginfo_t* info, void* /*ucontext*/) {
    std::string sigName;
    switch (sig) {
        case SIGSEGV: sigName = "Segmentation fault (SIGSEGV)"; break;
        case SIGBUS:  sigName = "Bus error (SIGBUS)"; break;
        case SIGFPE:  sigName = "Floating point exception (SIGFPE)"; break;
        case SIGILL:  sigName = "Illegal instruction (SIGILL)"; break;
        case SIGABRT: sigName = "Process abort (SIGABRT)"; break;
        default:      sigName = std::format("Signal {}", sig); break;
    }

    std::string faultAddr;
    if (info && (sig == SIGSEGV || sig == SIGBUS)) {
        faultAddr = std::format(" at address 0x{:X}", reinterpret_cast<uintptr_t>(info->si_addr));
    }

    std::string trace = CaptureStackTrace();
    std::string errorMsg = std::format("Fatal OS error: {}{}\n\nStack trace:\n{}", sigName, faultAddr, trace);

    if (t_posixCrashContext && t_posixCrashContext->outErrorMessage) {
        *t_posixCrashContext->outErrorMessage = std::move(errorMsg);
        siglongjmp(t_posixCrashContext->jumpBuf, 1);
    }

    pthread_exit(nullptr);
}

void InstallCrashHandler() {
    static bool s_installed = false;
    if (s_installed) {
        return;
    }
    s_installed = true;

    struct sigaction sa{};
    sa.sa_sigaction = CrashSignalHandler;
    sa.sa_flags = SA_SIGINFO | SA_NODEFER | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
}

static void SetupAlternateStack() {
    static thread_local std::array<char, 64 * 1024> t_altStack;
    static thread_local bool t_altStackInstalled = false;
    if (!t_altStackInstalled) {
        stack_t ss{};
        ss.ss_sp = t_altStack.data();
        ss.ss_size = t_altStack.size();
        ss.ss_flags = 0;
        sigaltstack(&ss, nullptr);
        t_altStackInstalled = true;
    }
}

bool RunProtected(const std::function<void()>& fn, std::string& outErrorMessage) {
    InstallCrashHandler();
    SetupAlternateStack();

    PosixCrashContext ctx;
    ctx.prev = t_posixCrashContext;
    ctx.outErrorMessage = &outErrorMessage;
    t_posixCrashContext = &ctx;

    if (sigsetjmp(ctx.jumpBuf, 1) != 0) {
        t_posixCrashContext = ctx.prev;
        return false;
    }

    try {
        fn();
    } catch (const std::exception& e) {
        outErrorMessage = std::format("Unhandled C++ exception: {}\n\nStack trace:\n{}", e.what(), CaptureStackTrace());
        t_posixCrashContext = ctx.prev;
        return false;
    } catch (...) {
        outErrorMessage = std::format("Unhandled unknown C++ exception\n\nStack trace:\n{}", CaptureStackTrace());
        t_posixCrashContext = ctx.prev;
        return false;
    }

    t_posixCrashContext = ctx.prev;
    return true;
}

void EnsurePlatformDependencies() {}

}  // namespace Frontend::Platform
