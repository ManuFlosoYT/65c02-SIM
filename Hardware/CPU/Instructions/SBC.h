#pragma once

namespace Hardware {
class CPU;
class Bus;
}


namespace Hardware::Instructions {

class SBC {
public:
    template <bool Debug>

    static void ExecuteImmediate(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void ExecuteABSX(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void ExecuteABSY(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void ExecuteINDX(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void ExecuteINDY(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void ExecuteIND_ZP(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions
