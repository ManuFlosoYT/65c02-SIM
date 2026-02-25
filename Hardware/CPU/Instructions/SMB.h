#pragma once

namespace Hardware {
class CPU;
class Bus;
}


namespace Hardware::Instructions {

class SMB {
public:
    static void Execute0(CPU& cpu, Bus& bus);
    static void Execute1(CPU& cpu, Bus& bus);
    static void Execute2(CPU& cpu, Bus& bus);
    static void Execute3(CPU& cpu, Bus& bus);
    static void Execute4(CPU& cpu, Bus& bus);
    static void Execute5(CPU& cpu, Bus& bus);
    static void Execute6(CPU& cpu, Bus& bus);
    static void Execute7(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions
