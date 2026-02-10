#!/bin/bash
set -e

list_programs() {
    echo "Available programs:"
    echo "  - eater (WOZMON / BASIC)"
    for f in Programas/*.c; do
        [ -f "$f" ] && echo "  - $(basename "${f%.c}") (C)"
    done
    for f in Programas/*.s; do
        [ -f "$f" ] && echo "  - $(basename "${f%.s}") (ASM)"
    done
}

if [ -z "$1" ]; then
    echo "Usage: $0 <program_name>"
    list_programs
    exit 1
fi

NAME=$1
mkdir -p Programas/build

if [ "$NAME" == "eater" ]; then
    echo "--- Compiling BASIC ---"
    (cd Linker/msbasic && chmod +x make.sh && ./make.sh) > /dev/null 2>&1
    cp Linker/msbasic/tmp/eater.bin Programas/build/eater.bin
    cp Linker/msbasic/tmp/eater.bin output/eater.bin

elif [ -f "SID/$NAME.s" ]; then
    echo "--- Assembling $NAME.s (ASM from SID/) ---"
    ca65 --cpu 65C02 -o "Programas/build/$NAME.o" "SID/$NAME.s"
    ld65 -C Linker/raw.cfg -o "Programas/build/$NAME.bin" "Programas/build/$NAME.o"
    mkdir -p output
    cp "Programas/build/$NAME.bin" "output/$NAME.bin"

elif [ -f "Programas/$NAME.s" ]; then
    echo "--- Assembling $NAME.s (ASM) ---"
    ca65 --cpu 65C02 -o "Programas/build/$NAME.o" "Programas/$NAME.s"
    ld65 -C Linker/raw.cfg -o "Programas/build/$NAME.bin" "Programas/build/$NAME.o"
    mkdir -p output
    cp "Programas/build/$NAME.bin" "output/$NAME.bin"

elif [ -f "Programas/$NAME.c" ]; then
    echo "--- Compiling $NAME.c (C) ---"
    cl65 -O -Oi -Or --static-locals --cpu 65C02 -t none -C Linker/memoria.cfg \
        -o "Programas/build/$NAME.bin" \
        -m "Programas/build/$NAME.map" \
        -l "Programas/build/$NAME.lst" \
        Linker/bios.s Linker/inicio.s "Programas/$NAME.c"
    mv "Programas/$NAME.o" "Programas/build/$NAME.o" 2>/dev/null || true
    cp "Programas/build/$NAME.bin" "output/$NAME.bin"

else
    echo "Error: Programas/$NAME.c or Programas/$NAME.s does not exist"
    list_programs
    exit 1
fi

echo "Compilation finished. Binary saved to output/$NAME.bin"