cmake_minimum_required(VERSION 4.1)
project(MIPS_SIM_65c02)

set(CMAKE_CXX_STANDARD 26)

add_executable(MIPS_SIM_65c02 main.cpp
        Instrucciones/Instruccion.cpp
        Instrucciones/Instruccion.h
        Instrucciones/NOP.cpp
        Instrucciones/NOP.h
        Componentes/CPU.cpp
        Componentes/CPU.h
        Componentes/Mem.cpp
        Componentes/Mem.h
        Componentes/Mem.h
)
