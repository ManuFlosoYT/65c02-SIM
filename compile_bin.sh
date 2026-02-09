#!/bin/bash
set -e

list_programs() {
    echo "Available programs:"
    echo "  - eater (WOZMON / BASIC)"
    for f in Programas/*.c; do
        echo "  - $(basename "${f%.c}") (C)"
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
else
    if [ ! -f "Programas/$NAME.c" ]; then
        echo "Error: Programas/$NAME.c does not exist"
        list_programs
        exit 1
    fi
    echo "--- Compiling $NAME.c ---"
    cl65 -O -Oi -Or --static-locals --cpu 65C02 -t none -C Linker/memoria.cfg \
        -o "Programas/build/$NAME.bin" \
        -m "Programas/build/$NAME.map" \
        -l "Programas/build/$NAME.lst" \
        Linker/bios.s Linker/inicio.s "Programas/$NAME.c"
    mv "Programas/$NAME.o" "Programas/build/$NAME.o" 2>/dev/null || true
    cp "Programas/build/$NAME.bin" "output/$NAME.bin"
fi

echo "Compilation finished. Binary saved to output/$NAME.bin"