#pragma once

#include <cstddef>
#include <cstdint>

namespace Control {
struct AppState;
}

namespace Frontend {

class SharedUrlLoader {
public:
    static void Init(Control::AppState* state);
    static void CheckUrlParameters();
    static bool LoadPayload(const uint8_t* data, size_t size);
};

}  // namespace Frontend
