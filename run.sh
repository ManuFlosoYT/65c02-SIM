#!/bin/bash
set -e

NAME=${1:?"Uso: $0 <nombre_bin_sin_extension> o basic"}
mkdir -p Programas/build

if [ "$NAME" == "basic" ]; then
    (cd Linker/msbasic && chmod +x make.sh && ./make.sh)
    cp Linker/msbasic/tmp/eater.bin Programas/build/basic.bin
else
    [ -f "Programas/$NAME.c" ] || { echo "Error: Programas/$NAME.c no existe"; exit 1; }
    cl65 -O --cpu 65C02 -t none -C Linker/memoria.cfg \
        -o "Programas/build/$NAME.bin" \
        -m "Programas/build/$NAME.map" \
        -l "Programas/build/$NAME.lst" \
        Linker/bios.s Linker/inicio.s "Programas/$NAME.c"
    mv "Programas/$NAME.o" "Programas/build/$NAME.o" 2>/dev/null || true
fi

./build/SIM_65C02 "$NAME"