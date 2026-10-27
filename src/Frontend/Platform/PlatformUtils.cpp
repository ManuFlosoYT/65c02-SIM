#include "Frontend/Platform/PlatformUtils.h"

#ifdef TARGET_WASM
#include <emscripten.h>
#include <string>

namespace Frontend::Platform {

bool OpenURL(std::string_view url) {
    if (url.empty()) {
        return false;
    }
    std::string nullTerminatedUrl(url);
    std::string script = "window.open('" + nullTerminatedUrl + "', '_blank');";
    emscripten_run_script(script.c_str());
    return true;
}

}  // namespace Frontend::Platform
#else
#include <SDL3/SDL.h>
#include <string>

namespace Frontend::Platform {

bool OpenURL(std::string_view url) {
    if (url.empty()) {
        return false;
    }
    std::string nullTerminatedUrl(url);
    return SDL_OpenURL(nullTerminatedUrl.c_str());
}

}  // namespace Frontend::Platform
#endif
