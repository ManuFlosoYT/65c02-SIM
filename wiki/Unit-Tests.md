# Unit Tests — Pruebas unitarias

**Directorio:** `UnitTests/`  
**Framework:** [Google Test](https://github.com/google/googletest)

## Descripción general

El directorio `UnitTests/` contiene una suite de tests unitarios que verifica el correcto funcionamiento de todas las instrucciones del procesador 65c02 y algunos componentes de hardware adicionales.

> ⚠️ La carpeta `UnitTests/` fue generada con asistencia de IA.

## Estructura

Cada instrucción tiene su propio archivo o directorio de tests:

```
UnitTests/
├── ADC/              ← Tests de ADC (varios archivos)
├── AND/
├── ASL/
├── BCC.cpp
├── BCS.cpp
├── BEQ.cpp
├── ...               ← Un .cpp por instrucción de branch
├── CMP/
├── SBC/
├── STA/
├── ...
├── Ampliados/        ← Tests de instrucciones 65C02 extendidas
├── LCD_Test.cpp      ← Tests del LCD (en algunos setups)
├── VIA_Test.cpp      ← Tests del VIA 6522
└── repro_freeze.cpp  ← Reproducción de un bug de congelación
```

## Patrón de test

Todos los tests siguen el mismo patrón con **Google Test fixtures**:

```cpp
#include <gtest/gtest.h>
#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class BEQ_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }
    Mem mem;
    CPU cpu;
};

// Probar BEQ cuando Z=0 (no salta)
TEST_F(BEQ_Test, BEQ_NoBranch_ZeroClear) {
    cpu.Z = 0;
    cpu.PC = 0x1000;
    mem.Write(0x1000, INS_BEQ);
    mem.Write(0x1001, 0x05);    // offset +5
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x10);
    cpu.Execute(mem);
    EXPECT_EQ(cpu.PC, 0x1003);  // No saltó: PC avanzó normalmente
}

// Probar BEQ cuando Z=1 (salta)
TEST_F(BEQ_Test, BEQ_Branch_ZeroSet) {
    cpu.Z = 1;
    cpu.PC = 0x1000;
    mem.Write(0x1000, INS_BEQ);
    mem.Write(0x1001, 0x05);    // offset +5
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x10);
    cpu.Execute(mem);
    EXPECT_EQ(cpu.PC, 0x1008);  // Saltó: 0x1002 + 5 = 0x1007 (JAM); fetch avanza PC a 0x1008
}
```

### Técnica de terminación (`INS_JAM`)

Los tests usan la instrucción `JAM` (también llamada `STP` o `KIL`) como **terminador**: al ejecutarla, `cpu.Execute()` devuelve 1 y el test puede verificar el estado final de los registros.

## Instrucciones con tests

### Tests simples (un archivo `.cpp`)

| Instrucción | Archivo |
|-------------|---------|
| BBR0–BBR7 | `BBR.cpp` |
| BBS0–BBS7 | `BBS.cpp` |
| BCC | `BCC.cpp` |
| BCS | `BCS.cpp` |
| BEQ | `BEQ.cpp` |
| BMI | `BMI.cpp` |
| BNE | `BNE.cpp` |
| BPL | `BPL.cpp` |
| BRA | `BRA.cpp` |
| BRK | `BRK.cpp` |
| BVC | `BVC.cpp` |
| BVS | `BVS.cpp` |
| CLC, CLD, CLI, CLV | `CLC.cpp`, `CLD.cpp`, `CLI.cpp`, `CLV.cpp` |
| DEX, DEY | `DEX.cpp`, `DEY.cpp` |
| INX, INY | `INX.cpp`, `INY.cpp` |
| NOP | `NOP.cpp` |
| PHA, PHP, PHX, PHY | `PHA.cpp`, `PHP.cpp`, `PHX.cpp`, `PHY.cpp` |
| PLA, PLP, PLX, PLY | `PLA.cpp`, `PLP.cpp`, `PLX.cpp`, `PLY.cpp` |
| RMB, SMB | `RMB.cpp`, `SMB.cpp` |
| RTI, RTS | `RTI.cpp`, `RTS.cpp` |
| SEC, SED, SEI | `SEC.cpp`, `SED.cpp`, `SEI.cpp` |
| TAX, TAY, TSX, TXA, TXS, TYA | `.cpp` individuales |

### Tests con directorio (múltiples modos de direccionamiento)

| Instrucción | Directorio | Modos testeados |
|-------------|-----------|-----------------|
| ADC | `ADC/` | Immediate, ZP, ZP_X, Absolute, Absolute_X, Absolute_Y, Indirect_X, Indirect_Y |
| AND | `AND/` | Todos los modos |
| ASL | `ASL/` | Accumulator, ZP, Absolute, etc. |
| CMP | `CMP/` | Todos los modos |
| CPX, CPY | `CPX/`, `CPY/` | Immediate, ZP, Absolute |
| DEC, INC | `DEC/`, `INC/` | ZP, Absolute, Accumulator |
| EOR | `EOR/` | Todos los modos |
| LDA, LDX, LDY | `LDA/`, `LDX/`, `LDY/` | Todos los modos |
| ROL, ROR | `ROL/`, `ROR/` | Accumulator, ZP, Absolute |
| SBC | `SBC/` | Todos los modos |
| STA, STX, STY, STZ | `STA/`, `STX/`, `STY/`, `STZ/` | Todos los modos |
| TRB, TSB | `TRB/`, `TSB/` | ZP, Absolute |

### Tests de hardware

| Archivo | Descripción |
|---------|-------------|
| `VIA_Test.cpp` | Prueba los timers T1/T2, registros DDR y callbacks del VIA 6522 |
| `repro_freeze.cpp` | Reproducción de un caso de congelación del emulador |

## Ejecutar los tests

Los tests se compilan y ejecutan automáticamente al final de `build-linux.sh`:

```bash
./build-linux.sh
```

Para ejecutar los tests manualmente después de compilar:

```bash
./output/linux/unit_tests
# O con filtro para una instrucción específica:
./output/linux/unit_tests --gtest_filter="BEQ*"
```

## Integración con CMake

Los tests están registrados como target `unit_tests` en `CMakeLists.txt` y enlazados con la biblioteca `65c02_core` y `googletest`.
