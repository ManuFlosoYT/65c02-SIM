#pragma once

#include "../CPU.h"
#include "../Mem.h"

namespace Hardware::Instructions {

class BBR {
public:
    static void Execute0(CPU& cpu, Mem& mem);
    static void Execute1(CPU& cpu, Mem& mem);
    static void Execute2(CPU& cpu, Mem& mem);
    static void Execute3(CPU& cpu, Mem& mem);
    static void Execute4(CPU& cpu, Mem& mem);
    static void Execute5(CPU& cpu, Mem& mem);
    static void Execute6(CPU& cpu, Mem& mem);
    static void Execute7(CPU& cpu, Mem& mem);
};

}  // namespace Hardware::Instructions
