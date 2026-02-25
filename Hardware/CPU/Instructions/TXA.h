#pragma once

namespace Hardware {
class CPU;
class Bus;
}


namespace Hardware::Instructions {

class TXA {
public:
    static void Execute(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions
