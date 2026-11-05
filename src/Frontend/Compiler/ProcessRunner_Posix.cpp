#ifndef TARGET_WASM
#include "ProcessRunner.h"
#include "CompilerVFS.h"
#include <cctype>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <spawn.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

static void WriteTextFile(const std::string& path, const std::string& txt) {
    std::ofstream out(path, std::ios::binary);
    if (out) {
        out << txt;
    }
}

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
