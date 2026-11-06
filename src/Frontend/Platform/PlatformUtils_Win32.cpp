#include "Frontend/Platform/PlatformUtils.h"

#include <SDL3/SDL.h>
#include <windows.h>
#include <csetjmp>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <utility>

namespace Frontend::Platform {

bool OpenURL(std::string_view url) {
    if (url.empty()) {
        return false;
    }
    std::string nullTerminatedUrl(url);
    return SDL_OpenURL(nullTerminatedUrl.c_str());
}

std::filesystem::path GetExecutableDirectory() {
    std::array<char, MAX_PATH> buffer{};
    if (GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size())) > 0) {
        return std::filesystem::path(buffer.data()).parent_path();
    }
    return std::filesystem::current_path();
}

char GetEnvPathDelimiter() {
    return ';';
}

std::string AppendExecutableExtension(std::string_view binaryName) {
    if (!binaryName.ends_with(".exe")) {
        return std::string(binaryName) + ".exe";
    }
    return std::string(binaryName);
}

std::string CaptureStackTrace() {
    std::array<void*, 62> stack{};
    WORD frames = CaptureStackBackTrace(0, 62, stack.data(), nullptr);
    std::string result;
    for (WORD i = 0; i < frames; ++i) {
        result += std::format("[{}] 0x{:016X}\n", i, reinterpret_cast<uintptr_t>(stack[i]));
    }
    return result.empty() ? "No stack trace available." : result;
}

struct WinCrashContext {
    jmp_buf jumpBuf;
    WinCrashContext* prev = nullptr;
    std::string* outErrorMessage = nullptr;
};

static thread_local WinCrashContext* t_winCrashContext = nullptr;

static LONG WINAPI VectoredCrashHandler(PEXCEPTION_POINTERS pExceptionInfo) {
    if (!pExceptionInfo || !pExceptionInfo->ExceptionRecord) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    DWORD code = pExceptionInfo->ExceptionRecord->ExceptionCode;
    if (code == EXCEPTION_ACCESS_VIOLATION ||
        code == EXCEPTION_ILLEGAL_INSTRUCTION ||
        code == EXCEPTION_INT_DIVIDE_BY_ZERO ||
        code == EXCEPTION_STACK_OVERFLOW ||
        code == EXCEPTION_PRIV_INSTRUCTION) {
        std::string codeStr;
        if (code == EXCEPTION_ACCESS_VIOLATION) codeStr = "Access Violation (0xC0000005)";
        else if (code == EXCEPTION_ILLEGAL_INSTRUCTION) codeStr = "Illegal Instruction (0xC000001D)";
        else if (code == EXCEPTION_INT_DIVIDE_BY_ZERO) codeStr = "Integer Divide by Zero (0xC0000094)";
        else if (code == EXCEPTION_STACK_OVERFLOW) codeStr = "Stack Overflow (0xC00000FD)";
        else codeStr = std::format("Hardware Exception (0x{:08X})", code);

        std::string trace = CaptureStackTrace();
        std::string errorMsg = std::format("Fatal OS error: {}\n\nStack trace:\n{}", codeStr, trace);

        if (t_winCrashContext && t_winCrashContext->outErrorMessage) {
            *t_winCrashContext->outErrorMessage = std::move(errorMsg);
            longjmp(t_winCrashContext->jumpBuf, 1);
        }

        ExitThread(1);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void InstallCrashHandler() {
    static bool s_installed = false;
    if (!s_installed) {
        AddVectoredExceptionHandler(1, VectoredCrashHandler);
        s_installed = true;
    }
}

bool RunProtected(const std::function<void()>& fn, std::string& outErrorMessage) {
    InstallCrashHandler();

    WinCrashContext ctx;
    ctx.prev = t_winCrashContext;
    ctx.outErrorMessage = &outErrorMessage;
    t_winCrashContext = &ctx;

    if (setjmp(ctx.jumpBuf) != 0) {
        t_winCrashContext = ctx.prev;
        return false;
    }

    try {
        fn();
    } catch (const std::exception& e) {
        outErrorMessage = std::format("Unhandled C++ exception: {}\n\nStack trace:\n{}", e.what(), CaptureStackTrace());
        t_winCrashContext = ctx.prev;
        return false;
    } catch (...) {
        outErrorMessage = std::format("Unhandled unknown C++ exception\n\nStack trace:\n{}", CaptureStackTrace());
        t_winCrashContext = ctx.prev;
        return false;
    }

    t_winCrashContext = ctx.prev;
    return true;
}

void EnsurePlatformDependencies() {
    const wchar_t* localAppData = _wgetenv(L"LOCALAPPDATA");
    std::filesystem::path baseDir;
    if (localAppData && *localAppData) {
        baseDir = localAppData;
    } else {
        const wchar_t* tempDir = _wgetenv(L"TEMP");
        if (tempDir && *tempDir) {
            baseDir = tempDir;
        } else {
            std::error_code ec;
            baseDir = std::filesystem::temp_directory_path(ec);
        }
    }

    std::filesystem::path binDir = baseDir / "65c02-SIM" / "bin";
    std::error_code ec;
    std::filesystem::create_directories(binDir, ec);

    constexpr std::array<std::pair<const wchar_t*, const wchar_t*>, 4> kDependencies = {{
        {L"AVUTIL_DLL", L"avutil-61.dll"},
        {L"SWSCALE_DLL", L"swscale-10.dll"},
        {L"AVCODEC_DLL", L"avcodec-63.dll"},
        {L"AVFORMAT_DLL", L"avformat-63.dll"}
    }};

    for (const auto& [resName, fileName] : kDependencies) {
        HRSRC hRes = FindResourceW(nullptr, resName, MAKEINTRESOURCEW(10));
        if (!hRes) {
            continue;
        }
        DWORD size = SizeofResource(nullptr, hRes);
        HGLOBAL hData = LoadResource(nullptr, hRes);
        if (!hData || size == 0) {
            continue;
        }
        const void* data = LockResource(hData);
        if (!data) {
            continue;
        }

        std::filesystem::path destPath = binDir / fileName;
        if (std::filesystem::exists(destPath, ec) && std::filesystem::file_size(destPath, ec) == size) {
            continue;
        }

        std::ofstream out(destPath, std::ios::binary | std::ios::trunc);
        if (out.is_open()) {
            out.write(reinterpret_cast<const char*>(data), size);
        }
    }

    SetDllDirectoryW(binDir.c_str());
}

}  // namespace Frontend::Platform
