# 65c02-SIM Wiki

Bienvenido a la wiki del **emulador 65c02** para el esquema de memoria de Ben Eater.

## ¿Qué es este proyecto?

Este proyecto es un emulador completo del microprocesador **WDC 65C02** (variante CMOS del clásico 6502 de MOS Technology) pensado para la disposición de memoria de los proyectos de [Ben Eater](https://eater.net/6502). Incluye una interfaz gráfica moderna basada en **ImGui** y una completa cadena de herramientas SDK para desarrollar software para el emulador.

## Componentes emulados

| Componente | Descripción |
|------------|-------------|
| **CPU** | Procesador WDC 65C02 completo |
| **RAM** | 32 KB de memoria de acceso aleatorio (`0x0000`–`0x7FFF`) |
| **ROM** | 32 KB de memoria de solo lectura (`0x8000`–`0xFFFF`) |
| **VIA** | MOS 6522 Versatile Interface Adapter (timers, I/O) |
| **ACIA** | MOS 6551 Asynchronous Communications Interface (serial) |
| **GPU** | Controlador gráfico personalizado con 7.5 KB de VRAM |
| **SID** | Chip de síntesis de sonido (3 osciladores + ADSR) |
| **LCD** | Pantalla LCD de 2×16 caracteres |

## Arquitectura general

```
┌──────────────────────────────────────────────────────┐
│              SIM_65C02 (Ejecutable Principal)         │
├──────────────────────────────────────────────────────┤
│                  Frontend / GUI                       │
│     ImGui · SDL3 · OpenGL 3.3 · ImGuiFileDialog      │
├──────────────────────────────────────────────────────┤
│                   Control Layer                       │
│           AppState · Console · UpdateChecker         │
├──────────────────────────────────────────────────────┤
│           Biblioteca estática 65c02_core              │
│  ┌───────────┬────────────┬──────────────────────┐   │
│  │  CPU      │  Memoria   │  Periféricos         │   │
│  │ (65c02)   │ (RAM+ROM)  │  VIA · ACIA · LCD    │   │
│  ├───────────┼────────────┼──────────────────────┤   │
│  │     GPU (VRAM 7.5 KB)  │  SID (síntesis)      │   │
│  └───────────┴────────────┴──────────────────────┘   │
├──────────────────────────────────────────────────────┤
│               SDK / Herramientas                      │
│   compile-bin.sh · image-to-bin.sh · midi-to-bin.sh  │
│     Linker configs · BIOS · Microsoft BASIC          │
├──────────────────────────────────────────────────────┤
│            Programas de ejemplo y Tests               │
│    Binaries/ (C + ASM)  ·  UnitTests/ (Google Test)  │
└──────────────────────────────────────────────────────┘
```

## Mapa de páginas de la wiki

- **Hardware y emulación**
  - [Hardware](Hardware) — Emulador principal y sistema de memoria
  - [CPU](CPU) — Procesador 65c02 y registros
  - [CPU Instructions](CPU-Instructions) — Referencia completa del juego de instrucciones
  - [GPU](GPU) — Procesador gráfico y VRAM
  - [SID](SID) — Síntesis de sonido
  - [VIA y ACIA](VIA-and-ACIA) — Chips de entrada/salida
  - [LCD](LCD) — Pantalla LCD
- **Interfaz de usuario**
  - [Frontend](Frontend) — Ventanas ImGui y controles
- **Desarrollo y SDK**
  - [Linker y Mapa de Memoria](Linker-and-Memory-Layout) — Configuraciones de enlazado, BIOS, mapa de memoria
  - [SDK](SDK) — Herramientas de compilación y conversión
  - [Binaries](Binaries) — Programas de ejemplo incluidos
- **Construcción y pruebas**
  - [Building](Building) — Compilar el proyecto desde el código fuente
  - [Unit Tests](Unit-Tests) — Infraestructura de pruebas unitarias

## Inicio rápido

### Ejecutar el simulador (binarios precompilados)

1. Descarga la última versión desde la [página de releases](https://github.com/ManuFlosoYT/65c02-SIM/releases).
2. Descarga también el `SDK.zip` si quieres desarrollar programas.
3. Abre el simulador y selecciona el archivo `.bin` que deseas ejecutar.

### Compilar un programa desde el SDK

```bash
# Compilar un programa en C o ensamblador
./compile-bin.sh <nombre_programa>

# Convertir una imagen a formato VRAM
./image-to-bin.sh <imagen>

# Convertir un archivo MIDI a código SID
./midi-to-bin.sh <archivo_midi>
```

Consulta la página [SDK](SDK) para más detalles.

## Licencia

El código fuente C++ del emulador está licenciado bajo la **MIT License**.  
El proyecto incluye Microsoft BASIC (© 1977 Microsoft) y WozMon (© 1976 Apple Computer, Inc.) con fines educativos.
