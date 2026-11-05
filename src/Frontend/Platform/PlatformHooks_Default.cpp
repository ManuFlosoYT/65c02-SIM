#include "Frontend/Platform/PlatformHooks.h"

namespace Frontend::Platform {

void PreInitializeGraphics() {
}

bool RetryInitializeGraphics(const std::function<bool()>& /*retryFn*/) {
    return false;
}

void TrimProcessMemory() {
}

}  // namespace Frontend::Platform
