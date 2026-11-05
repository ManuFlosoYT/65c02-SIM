#ifndef TARGET_WASM
#include "ProcessRunner.h"
#include "CompilerVFS.h"
#include <fstream>
#include <string>
#include <windows.h>

static void WriteTextFile(const std::string& path, const std::string& txt) {
    std::ofstream out(path, std::ios::binary);
    if (out) {
        out << txt;
    }
}

int ProcessRunner::PlatformRunCommand(const std::string& cmd, const std::string& logFile) {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = nullptr;

    HANDLE hLog = CreateFileA(logFile.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &saAttr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hLog == INVALID_HANDLE_VALUE) {
        return -1;
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = hLog;
    si.hStdOutput = hLog;
    si.dwFlags |= STARTF_USESTDHANDLES;
    ZeroMemory(&pi, sizeof(pi));

    std::string mutableCmd = cmd;
    if (!CreateProcessA(nullptr, mutableCmd.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hLog);
        WriteTextFile(logFile, "Error: Failed to execute process: error code " + std::to_string(GetLastError()) + "\n");
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hLog);

    return static_cast<int>(exitCode);
}
#endif
