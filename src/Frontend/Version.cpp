#include "Frontend/Version.h"

#include <sstream>

namespace Frontend {

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

}  // namespace Frontend
