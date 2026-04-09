#pragma once

namespace Hardware {
class CPU;
class Bus;
}


namespace Hardware::Instructions {

class BBR {
public:
    template <bool Debug>

    static void Execute0(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void Execute1(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void Execute2(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void Execute3(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void Execute4(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void Execute5(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void Execute6(CPU& cpu, Bus& bus);
    template <bool Debug>

    static void Execute7(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions
