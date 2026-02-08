#!/bin/bash
set -e

NAME=${1:?"Usage: $0 <bin_name_without_extension> or eater"}
mkdir -p Programas/build

if [ "$NAME" == "eater" ]; then
    echo "--- Compiling BASIC ---"
    (cd Linker/msbasic && chmod +x make.sh && ./make.sh) > /dev/null 2>&1
    cp Linker/msbasic/tmp/eater.bin Programas/build/eater.bin
    cp Linker/msbasic/tmp/eater.bin output/eater.bin
else
    [ -f "Programas/$NAME.c" ] || { echo "Error: Programas/$NAME.c does not exist"; exit 1; }
    echo "--- Compiling $NAME.c ---"
    cl65 -O --cpu 65C02 -t none -C Linker/memoria.cfg \
        -o "Programas/build/$NAME.bin" \
        -m "Programas/build/$NAME.map" \
        -l "Programas/build/$NAME.lst" \
        Linker/bios.s Linker/inicio.s "Programas/$NAME.c"
    mv "Programas/$NAME.o" "Programas/build/$NAME.o" 2>/dev/null || true
    cp "Programas/build/$NAME.bin" "output/$NAME.bin"
fi

echo "Compilation finished. Binary saved to output/$NAME.bin"