#include "UpdateChecker.h"

#include <httplib.h>

#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

namespace Frontend {

using json = nlohmann::json;

Version Version::Parse(const std::string& versionStr) {
    Version version;
    version.tag = versionStr;
    std::string cleanVer = versionStr;

    if (!cleanVer.empty() && cleanVer[0] == 'v') {
        cleanVer = cleanVer.substr(1);
    }

    size_t dashPos = cleanVer.find('-');
    if (dashPos != std::string::npos) {
        cleanVer = cleanVer.substr(0, dashPos);
    }

    std::stringstream stringStream(cleanVer);
    char dot = 0;
    stringStream >> version.major >> dot >> version.minor >> dot >> version.patch;
    return version;
}

bool Version::operator>(const Version& other) const {
    if (major != other.major) {
        return major > other.major;
    }
    if (minor != other.minor) {
        return minor > other.minor;
    }
    return patch > other.patch;
}

void UpdateChecker::CheckForUpdates(const std::string& currentVersion, const UpdateCallback& callback) {
    std::thread([currentVersion, callback]() {
        std::string latestTag = FetchLatestReleaseTag();

        if (latestTag.empty()) {
            return;
        }

        Version current = Version::Parse(currentVersion);
        Version latest = Version::Parse(latestTag);

        bool updateAvailable = latest > current;
        callback(updateAvailable, latestTag);
    }).detach();
}

std::string UpdateChecker::FetchLatestReleaseTag() {
    httplib::Client cli("https://api.github.com");

    cli.set_connection_timeout(5);
    cli.set_read_timeout(5);

    httplib::Headers headers = {{"User-Agent", "65c02-SIM-UpdateChecker"},
                                {"Accept", "application/vnd.github.v3+json"}};
    auto res = cli.Get("/repos/ManuFlosoYT/65c02-SIM/releases/latest", headers);

    if (res && res->status == 200) {
        try {
            auto jsonResponse = json::parse(res->body);
            if (jsonResponse.contains("tag_name")) {
                return jsonResponse["tag_name"];
            }
        } catch (const std::exception& exception) {
            std::cerr << "Failed to parse update json: " << exception.what() << '\n';
        }
    }

    return "";
}

}  // namespace Frontend
