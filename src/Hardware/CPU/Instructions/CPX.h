#pragma once

namespace Hardware {
class CPU;
class Bus;
}


namespace Hardware::Instructions {

class CPX {
public:
    template <bool Debug>

    static void ExecuteImmediate(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void ExecuteABS(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions
