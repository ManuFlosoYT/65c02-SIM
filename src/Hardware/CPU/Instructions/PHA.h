#pragma once

namespace Hardware {
class CPU;
class Bus;
}


namespace Hardware::Instructions {

class PHA {
public:
    template <bool Debug>

    static void Execute(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions
