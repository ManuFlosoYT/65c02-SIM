#include "Frontend/Platform/PlatformHooks.h"

#include <SDL3/SDL.h>
#include <malloc.h>
#include <iostream>
#include <string_view>

namespace Frontend::Platform {

void PreInitializeGraphics() {
    if (SDL_getenv("__NV_PRIME_RENDER_OFFLOAD") != nullptr && SDL_getenv("SDL_VIDEO_DRIVER") == nullptr) {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
    }
}

bool RetryInitializeGraphics(const std::function<bool()>& retryFn) {
    const char* envDriver = SDL_getenv("SDL_VIDEO_DRIVER");
    const char* hintDriver = SDL_GetHint(SDL_HINT_VIDEO_DRIVER);
    bool alreadyX11 = (envDriver != nullptr && std::string_view(envDriver) == "x11") ||
                      (hintDriver != nullptr && std::string_view(hintDriver) == "x11");
    if (!alreadyX11) {
        std::cerr << "Retrying graphics initialization with SDL_VIDEO_DRIVER=x11...\n";
        SDL_Quit();
        SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "x11", SDL_HINT_OVERRIDE);
        return retryFn();
    }
    return false;
}

void TrimProcessMemory() {
    malloc_trim(0);
}

}  // namespace Frontend::Platform
