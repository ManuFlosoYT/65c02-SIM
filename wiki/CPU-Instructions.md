# CPU Instructions — Juego de instrucciones del 65C02

El emulador implementa el juego completo de instrucciones del **WDC 65C02**, incluyendo las instrucciones adicionales respecto al 6502 NMOS original.

Cada instrucción tiene su propia clase en `Hardware/CPU/Instructions/`. El despacho se realiza mediante una tabla `switch` en `Hardware/CPU/Dispatch.cpp`.

## Instrucciones por categoría

### Aritmética

| Mnemónico | Descripción | Archivo |
|-----------|-------------|---------|
| `ADC` | Add with Carry — suma al acumulador con acarreo | `ADC.h/cpp` |
| `SBC` | Subtract with Carry — resta del acumulador con acarreo | `SBC.h/cpp` |
| `INC` | Increment — incrementa memoria o acumulador | `INC.h/cpp` |
| `DEC` | Decrement — decrementa memoria o acumulador | `DEC.h/cpp` |
| `INX` | Increment X | `INX.h/cpp` |
| `INY` | Increment Y | `INY.h/cpp` |
| `DEX` | Decrement X | `DEX.h/cpp` |
| `DEY` | Decrement Y | `DEY.h/cpp` |

### Lógica y desplazamientos

| Mnemónico | Descripción | Archivo |
|-----------|-------------|---------|
| `AND` | AND lógico con el acumulador | `AND.h/cpp` |
| `ORA` | OR inclusivo con el acumulador | `ORA.h/cpp` |
| `EOR` | OR exclusivo (XOR) con el acumulador | `EOR.h/cpp` |
| `ASL` | Arithmetic Shift Left | `ASL.h/cpp` |
| `LSR` | Logical Shift Right | `LSR.h/cpp` |
| `ROL` | Rotate Left (a través de Carry) | `ROL.h/cpp` |
| `ROR` | Rotate Right (a través de Carry) | `ROR.h/cpp` |

### Carga y almacenamiento

| Mnemónico | Descripción | Archivo |
|-----------|-------------|---------|
| `LDA` | Load Accumulator | `LDA.h/cpp` |
| `LDX` | Load X | `LDX.h/cpp` |
| `LDY` | Load Y | `LDY.h/cpp` |
| `STA` | Store Accumulator | `STA.h/cpp` |
| `STX` | Store X | `STX.h/cpp` |
| `STY` | Store Y | `STY.h/cpp` |
| `STZ` | Store Zero (65C02) | `STZ.h/cpp` |

### Transferencias de registros

| Mnemónico | Descripción | Archivo |
|-----------|-------------|---------|
| `TAX` | Transfer A → X | `TAX.h/cpp` |
| `TAY` | Transfer A → Y | `TAY.h/cpp` |
| `TXA` | Transfer X → A | `TXA.h/cpp` |
| `TYA` | Transfer Y → A | `TYA.h/cpp` |
| `TSX` | Transfer SP → X | `TSX.h/cpp` |
| `TXS` | Transfer X → SP | `TXS.h/cpp` |

### Pila

| Mnemónico | Descripción | Archivo |
|-----------|-------------|---------|
| `PHA` | Push Accumulator | `PHA.h/cpp` |
| `PLA` | Pull Accumulator | `PLA.h/cpp` |
| `PHP` | Push Processor status | `PHP.h/cpp` |
| `PLP` | Pull Processor status | `PLP.h/cpp` |
| `PHX` | Push X (65C02) | `PHX.h/cpp` |
| `PLX` | Pull X (65C02) | `PLX.h/cpp` |
| `PHY` | Push Y (65C02) | `PHY.h/cpp` |
| `PLY` | Pull Y (65C02) | `PLY.h/cpp` |

### Ramas (saltos condicionales)

| Mnemónico | Condición | Archivo |
|-----------|-----------|---------|
| `BCC` | Branch if Carry Clear (C=0) | `BCC.h/cpp` |
| `BCS` | Branch if Carry Set (C=1) | `BCS.h/cpp` |
| `BEQ` | Branch if Equal / Zero Set (Z=1) | `BEQ.h/cpp` |
| `BNE` | Branch if Not Equal / Zero Clear (Z=0) | `BNE.h/cpp` |
| `BMI` | Branch if Minus (N=1) | `BMI.h/cpp` |
| `BPL` | Branch if Plus (N=0) | `BPL.h/cpp` |
| `BVC` | Branch if Overflow Clear (V=0) | `BVC.h/cpp` |
| `BVS` | Branch if Overflow Set (V=1) | `BVS.h/cpp` |
| `BRA` | Branch Always (65C02) | `BRA.h/cpp` |

### Control de flujo

| Mnemónico | Descripción | Archivo |
|-----------|-------------|---------|
| `JMP` | Jump — salto incondicional | `JMP.h/cpp` |
| `JSR` | Jump to Subroutine | `JSR.h/cpp` |
| `RTS` | Return from Subroutine | `RTS.h/cpp` |
| `RTI` | Return from Interrupt | `RTI.h/cpp` |
| `BRK` | Break — interrupción software | `BRK.h/cpp` |
| `NOP` | No Operation | `NOP.h/cpp` |

### Flags de estado

| Mnemónico | Descripción | Archivo |
|-----------|-------------|---------|
| `CLC` | Clear Carry | `CLC.h/cpp` |
| `SEC` | Set Carry | `SEC.h/cpp` |
| `CLI` | Clear Interrupt Disable | `CLI.h/cpp` |
| `SEI` | Set Interrupt Disable | `SEI.h/cpp` |
| `CLD` | Clear Decimal | `CLD.h/cpp` |
| `SED` | Set Decimal | `SED.h/cpp` |
| `CLV` | Clear Overflow | `CLV.h/cpp` |

### Comparaciones

| Mnemónico | Descripción | Archivo |
|-----------|-------------|---------|
| `CMP` | Compare Accumulator | `CMP.h/cpp` |
| `CPX` | Compare X | `CPX.h/cpp` |
| `CPY` | Compare Y | `CPY.h/cpp` |
| `BIT` | Bit Test | `BIT.h/cpp` |

### Manipulación de bits (65C02)

Instrucciones exclusivas del WDC 65C02 para operar sobre bits individuales en la página cero:

| Mnemónico | Descripción | Archivo |
|-----------|-------------|---------|
| `BBR` | Branch if Bit Reset — salta si el bit N de la dirección de página cero es 0 | `BBR.h/cpp` |
| `BBS` | Branch if Bit Set — salta si el bit N de la dirección de página cero es 1 | `BBS.h/cpp` |
| `RMB` | Reset Memory Bit — pone a 0 el bit N de la dirección de página cero | `RMB.h/cpp` |
| `SMB` | Set Memory Bit — pone a 1 el bit N de la dirección de página cero | `SMB.h/cpp` |
| `TSB` | Test and Set Bits | `TSB.h/cpp` |
| `TRB` | Test and Reset Bits | `TRB.h/cpp` |

## Modos de direccionamiento soportados

| Modo | Ejemplo | Descripción |
|------|---------|-------------|
| Implícito | `CLC` | Sin operando |
| Acumulador | `ASL A` | Opera sobre el acumulador |
| Inmediato | `LDA #$42` | Valor constante |
| Página cero | `LDA $10` | Dirección en `0x0000`–`0x00FF` |
| Página cero, X | `LDA $10,X` | Página cero indexado por X |
| Página cero, Y | `LDA $10,Y` | Página cero indexado por Y |
| Absoluto | `LDA $1234` | Dirección de 16 bits |
| Absoluto, X | `LDA $1234,X` | Absoluto indexado por X |
| Absoluto, Y | `LDA $1234,Y` | Absoluto indexado por Y |
| Indirecto | `JMP ($1234)` | Indirección a través de puntero |
| (Indirecto, X) | `LDA ($10,X)` | Indirecto indexado por X |
| (Indirecto), Y | `LDA ($10),Y` | Indirecto indexado por Y post |
| (Indirecto) página cero | `LDA ($10)` | Indirecto página cero (65C02) |
| Relativo | `BEQ label` | Offset relativo de 8 bits |
| Absoluto Indirecto X | `JMP ($1234,X)` | Indirecto absoluto indexado (65C02) |

## Precisión de ciclos

Cuando el modo ciclo-exacto está activado (`SetCycleAccurate(true)`), cada instrucción consume el número correcto de ciclos del 65C02. Los accesos que cruzan límites de página añaden un ciclo extra de penalización.
