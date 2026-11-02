#pragma once

#include <string>
#include <string_view>

namespace Hardware {

class SHA256 {
   public:
    static std::string HashHex(std::string_view input);
};

}  // namespace Hardware
