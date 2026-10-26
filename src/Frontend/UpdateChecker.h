#pragma once

#include <functional>
#include <string>

#include "Frontend/Version.h"

namespace Frontend {

class UpdateChecker {
   public:
    using UpdateCallback = std::function<void(bool updateAvailable, const std::string& latestVersion)>;

#ifndef TARGET_WASM
    static void CheckForUpdates(const std::string& currentVersion, const UpdateCallback& callback);
#endif

   private:
    static std::string FetchLatestReleaseTag();
};

}  // namespace Frontend
