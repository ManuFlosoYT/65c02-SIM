#include "UpdateChecker.h"

#include <nlohmann/json.hpp>
#include <thread>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <array>
    #include <cstdio>
    #include <memory>
#endif

using json = nlohmann::json;

Version Version::Parse(const std::string& versionStr) {
    Version v;
    v.tag = versionStr;
    std::string cleanVer = versionStr;
    if (!cleanVer.empty() && cleanVer[0] == 'v') {
        cleanVer = cleanVer.substr(1);
    }

    // Handle "dirty" or commit hash suffixes if present (e.g. 1.2.0-dirty)
    size_t dashPos = cleanVer.find('-');
    if (dashPos != std::string::npos) {
        cleanVer = cleanVer.substr(0, dashPos);
    }

    std::sscanf(cleanVer.c_str(), "%d.%d.%d", &v.major, &v.minor, &v.patch);
    return v;
}

bool Version::operator>(const Version& other) const {
    if (major != other.major) return major > other.major;
    if (minor != other.minor) return minor > other.minor;
    return patch > other.patch;
}

void UpdateChecker::CheckForUpdates(const std::string& currentVersion,
                                    UpdateCallback callback) {
    std::thread([currentVersion, callback]() {
        std::string latestTag = FetchLatestReleaseTag();
        if (latestTag.empty()) {
            // Silent failure
            return;
        }

        Version current = Version::Parse(currentVersion);
        Version latest = Version::Parse(latestTag);

        bool updateAvailable = latest > current;
        callback(updateAvailable, latestTag);
    }).detach();
}

#ifdef _WIN32
// Windows implementation using CreateProcess to avoid console window
std::string UpdateChecker::FetchLatestReleaseTag() {
    std::string cmd =
        "curl -s "
        "https://api.github.com/repos/ManuFlosoYT/65c02-SIM/releases/latest";

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;

    if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0))
        return "";
    SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hChildStd_OUT_Wr;
    si.hStdError = NULL;       // We don't care about errors, silent fail
    si.wShowWindow = SW_HIDE;  // Ensure hidden

    ZeroMemory(&pi, sizeof(pi));

    // Create the child process.
    // CreateProcess requires mutable string
    std::vector<char> cmdVec(cmd.begin(), cmd.end());
    cmdVec.push_back(0);

    if (!CreateProcessA(NULL, cmdVec.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW,
                        NULL, NULL, &si, &pi)) {
        CloseHandle(hChildStd_OUT_Wr);
        CloseHandle(hChildStd_OUT_Rd);
        return "";
    }

    CloseHandle(hChildStd_OUT_Wr);  // Close write end in parent

    std::string output;
    DWORD dwRead;
    CHAR chBuf[4096];
    bool bSuccess = FALSE;

    while (true) {
        bSuccess =
            ReadFile(hChildStd_OUT_Rd, chBuf, sizeof(chBuf) - 1, &dwRead, NULL);
        if (!bSuccess || dwRead == 0) break;
        chBuf[dwRead] = 0;
        output += chBuf;
    }

    CloseHandle(hChildStd_OUT_Rd);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    try {
        auto j = json::parse(output);
        if (j.contains("tag_name")) {
            return j["tag_name"];
        }
    } catch (...) {
    }

    return "";
}
#else
// Linux/Unix implementation using popen
std::string UpdateChecker::FetchLatestReleaseTag() {
    std::string cmd =
        "curl -s "
        "https://api.github.com/repos/ManuFlosoYT/65c02-SIM/releases/latest";
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"),
                                                  pclose);

    if (!pipe) return "";  // Silent fail

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    try {
        auto j = json::parse(result);
        if (j.contains("tag_name")) {
            return j["tag_name"];
        }
    } catch (...) {
    }

    return "";
}
#endif
