#ifndef TARGET_WASM
#include "ProcessRunner.h"
#include "CompilerVFS.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

static std::string ReadFile(const std::string& path) {
    std::ifstream fileIn(path, std::ios::binary);
    if (!fileIn) {
        return "";
    }
    std::ostringstream contents;
    contents << fileIn.rdbuf();
    return contents.str();
}

static void WriteTextFile(const std::string& path, const std::string& txt) {
    std::ofstream out(path, std::ios::binary);
    if (out) {
        out << txt;
    }
}

#ifdef _WIN32
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
#else
int ProcessRunner::PlatformRunCommand(const std::string& cmd, const std::string& logFile) {
    std::vector<std::string> args;
    std::string current;
    bool inQuotes = false;
    for (char c : cmd) {
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (std::isspace(static_cast<unsigned char>(c)) && !inQuotes) {
            if (!current.empty()) {
                args.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) {
        args.push_back(current);
    }

    if (args.empty()) {
        return -1;
    }

    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) != 0) {
        return -1;
    }
    posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, logFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO);

    std::vector<std::vector<char>> argBuffers;
    argBuffers.reserve(args.size());
    for (const auto& arg : args) {
        std::vector<char> buf(arg.begin(), arg.end());
        buf.push_back('\0');
        argBuffers.push_back(std::move(buf));
    }

    std::vector<char*> argv;
    argv.reserve(argBuffers.size() + 1);
    for (auto& buf : argBuffers) {
        argv.push_back(buf.data());
    }
    argv.push_back(nullptr);

    pid_t pid = 0;
    int spawnResult = posix_spawnp(&pid, argv.at(0), &actions, nullptr, argv.data(), nullptr);
    if (spawnResult != 0) {
        posix_spawn_file_actions_destroy(&actions);
        WriteTextFile(logFile, "Error: Failed to execute compiler '" + args.at(0) + "': " + std::strerror(spawnResult) + "\n");
        return -1;
    }

    int status = 0;
    waitpid(pid, &status, 0);
    posix_spawn_file_actions_destroy(&actions);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
#endif

int ProcessRunner::RunCommand(const std::string& cmd, std::string& output) {
    std::string tempLog = CompilerVFS::GetTempDir() + "/temp.log";
    int result = PlatformRunCommand(cmd, tempLog);
    output = ReadFile(tempLog);
    return result;
}

#endif
