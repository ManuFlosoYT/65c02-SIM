#pragma once

#include <functional>

namespace Core {
class Emulator;
}

namespace System::Hardware::Scripting::Lang {
class Interpreter;
}

namespace Hardware {

class EmulatorScriptBindings {
public:
    static void Register(
        System::Hardware::Scripting::Lang::Interpreter& interp,
        Core::Emulator& emu,
        std::function<bool()> abortCheck = nullptr
    );
};

} // namespace Hardware
