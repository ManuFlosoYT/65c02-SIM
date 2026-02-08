#!/bin/bash
set -e
echo Compiling program and unit tests
cmake -S . -B build
cmake --build build -j$(nproc)
echo Running unit tests
./build/unit_tests

mkdir -p output
cp build/SIM_65C02_CLI output/ 2>/dev/null || true
cp build/SIM_65C02_GUI output/ 2>/dev/null || true
echo "Build executables copied to output/"
