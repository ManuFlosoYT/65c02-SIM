#pragma once

#include <string>

namespace Frontend {

struct Version {
    int major = 0;
    int minor = 0;
    int patch = 0;
    std::string tag;

    static Version Parse(const std::string& versionStr);
    bool operator>(const Version& other) const;
};

}  // namespace Frontend
