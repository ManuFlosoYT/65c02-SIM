#!/bin/bash
set -e

if [ -z "$1" ]; then
    echo "Uso: ./run.sh <nombre_bin_sin_extension>"
    exit 1
fi

mkdir -p ./Programas/build

SOURCE="./Programas/$1.c"
if [ ! -f "$SOURCE" ]; then
    echo "Error: El fichero $SOURCE no existe. Revisa que este en la carpeta Programas y que el nombre sea correcto (sin extension)"
    exit 1
fi

echo "--- Compilando $1 ---"

EXTRA_SOURCES=""
ASM_FLAGS=""

if [ "$1" == "basic" ]; then
    EXTRA_SOURCES="Linker/msbasic/msbasic.s"
    ASM_FLAGS="--asm-define eater -I Linker/msbasic"
fi

cl65 -O --cpu 65C02 -t none -C Linker/memoria.cfg $ASM_FLAGS -o "./Programas/build/$1.bin" -m "./Programas/build/$1.map" -l "./Programas/build/$1.lst" Linker/bios.s Linker/inicio.s $EXTRA_SOURCES "$SOURCE"
mv "./Programas/$1.o" "./Programas/build/$1.o"
echo "Binario compilado correctamente"
echo ""

# Ejecutar el simulador
./build/SIM_65C02 "$1"
