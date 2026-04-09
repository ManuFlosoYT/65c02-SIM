#pragma once

namespace Hardware {
class CPU;
class Bus;
}


namespace Hardware::Instructions {

class JMP {
public:
    template <bool Debug>

    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void ExecuteIND(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void ExecuteABSX(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions
