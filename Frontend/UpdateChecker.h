#pragma once

#include <string>
#include <functional>

namespace Frontend {

struct Version {
    int major = 0;
    int minor = 0;
    int patch = 0;
    std::string tag;

    static Version Parse(const std::string& versionStr);
    bool operator>(const Version& other) const;
};

class UpdateChecker {
public:
    using UpdateCallback = std::function<void(bool updateAvailable, const std::string& latestVersion)>;

    static void CheckForUpdates(const std::string& currentVersion, UpdateCallback callback);

private:
    static std::string FetchLatestReleaseTag();
};

}  // namespace Frontend
