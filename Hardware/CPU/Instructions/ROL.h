#pragma once

namespace Hardware {
class CPU;
class Bus;
}


namespace Hardware::Instructions {

class ROL {
public:
    template <bool Debug>

    static void ExecuteAccumulator(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void ExecuteABSX(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions
