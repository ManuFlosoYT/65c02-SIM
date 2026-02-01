#!/bin/bash
echo Compilando programa y pruebas unitarias
cmake -S . -B build
cmake --build build -j$(nproc)
echo Lanzando pruebas unitarias
./build/unit_tests
