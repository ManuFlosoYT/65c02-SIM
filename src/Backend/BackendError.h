#pragma once

#include <string>
#include <string_view>

namespace Backend {

struct BackendErrorState {
    bool active = false;
    bool isFatal = false;
    std::string summary;
    std::string stackTrace;

    void Set(std::string_view msg, std::string_view trace = {}, bool fatal = false) {
        active = true;
        isFatal = fatal;
        summary = std::string(msg);
        stackTrace = std::string(trace);
    }

    void Clear() noexcept {
        active = false;
        isFatal = false;
        summary.clear();
        stackTrace.clear();
    }
};

}  // namespace Backend
