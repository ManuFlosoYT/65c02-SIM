#pragma once

namespace Hardware {
class CPU;
class Bus;
}


namespace Hardware::Instructions {

class LSR {
public:
    static void ExecuteAccumulator(CPU& cpu, Bus& bus);
    static void ExecuteZP(CPU& cpu, Bus& bus);
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    static void ExecuteABS(CPU& cpu, Bus& bus);
    static void ExecuteABSX(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions
