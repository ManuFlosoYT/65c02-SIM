#!/bin/bash

if [ -z "$1" ]; then
    echo "Uso: ./run.sh <nombre_bin_sin_extension>"
    exit 1
fi

BIN_FILE="./Programas/build/$1.bin"

if [ ! -f "$BIN_FILE" ]; then
    echo "Error: El archivo $BIN_FILE no existe."
    exit 1
fi

time ./build/SIM_65C02 "$1"
