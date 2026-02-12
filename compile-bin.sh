#!/bin/bash
set -e

list_programs() {
    echo "Available programs:"
    echo "  - eater (WOZMON / BASIC)"
    for f in Binaries/*.c; do
        [ -f "$f" ] && echo "  - $(basename "${f%.c}") (C)"
    done
    for f in Binaries/*.s; do
        [ -f "$f" ] && echo "  - $(basename "${f%.s}") (ASM)"
    done
}

if [ -z "$1" ]; then
    echo "Usage: $0 <program_name>"
    list_programs
    exit 1
fi

NAME=$1
mkdir -p Binaries/build


if [ "$NAME" == "eater" ]; then
    echo "--- Compiling BASIC ---"
    (cd Linker/msbasic && chmod +x make.sh && ./make.sh) > /dev/null 2>&1
    cp Linker/msbasic/tmp/eater.bin Binaries/build/eater.bin
    mkdir -p output/rom
    cp Linker/msbasic/tmp/eater.bin output/rom/eater.bin

elif [ -f "SID/$NAME.s" ]; then
    echo "--- Assembling $NAME.s (ASM from SID/) ---"
    ca65 --cpu 65C02 -o "Binaries/build/$NAME.o" "SID/$NAME.s"
    ld65 -C Linker/raw.cfg -o "Binaries/build/$NAME.bin" "Binaries/build/$NAME.o"
    mkdir -p output/rom
    cp "Binaries/build/$NAME.bin" "output/rom/$NAME.bin"

elif [ -f "Binaries/$NAME.s" ]; then
    echo "--- Assembling $NAME.s (ASM) ---"
    ca65 --cpu 65C02 -o "Binaries/build/$NAME.o" "Binaries/$NAME.s"
    ld65 -C Linker/raw.cfg -o "Binaries/build/$NAME.bin" "Binaries/build/$NAME.o"
    mkdir -p output/rom
    cp "Binaries/build/$NAME.bin" "output/rom/$NAME.bin"

elif [ -f "Binaries/$NAME.c" ]; then
    echo "--- Compiling $NAME.c (C) ---"
    cl65 -O -Oi -Or --static-locals --cpu 65C02 -t none -C Linker/C-Runtime.cfg \
        -o "Binaries/build/$NAME.bin" \
        -m "Binaries/build/$NAME.map" \
        -l "Binaries/build/$NAME.lst" \
        Linker/bios.s Linker/C-Runtime.s "Binaries/$NAME.c"
    mv "Binaries/$NAME.o" "Binaries/build/$NAME.o" 2>/dev/null || true
    mkdir -p output/rom
    cp "Binaries/build/$NAME.bin" "output/rom/$NAME.bin"

else
    echo "Error: Binaries/$NAME.c or Binaries/$NAME.s does not exist"
    list_programs
    exit 1
fi

echo "Compilation finished. Binary saved to output/rom/$NAME.bin"
exit 0