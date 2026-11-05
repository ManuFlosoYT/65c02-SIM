#pragma once

#include <functional>

namespace Frontend::Platform {

void PreInitializeGraphics();
bool RetryInitializeGraphics(const std::function<bool()>& retryFn);
void TrimProcessMemory();

}  // namespace Frontend::Platform
