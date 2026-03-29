#!/bin/bash
set -e

list_programs() {
    echo "Available programs:"
    echo "  - all (Compile all programs)"
    echo "  - eater (WOZMON / BASIC)"
    for f in Binaries/*.c; do
        [ -f "$f" ] && echo "  - $(basename "${f%.c}") (C)"
    done
    for f in Binaries/*.s; do
        [ -f "$f" ] && echo "  - $(basename "${f%.s}") (ASM)"
    done
}

if [ -z "$1" ]; then
    echo "Usage: $0 <program_name> [--microDOS]"
    list_programs
    exit 1
fi

NAME=$1

# --microDOS: compile a .c file as a microDOS .app binary
if [ "$2" == "--microDOS" ]; then
    echo "--- Compiling $NAME.app (microDOS App) ---"
    SRC=""
    if [ -f "Binaries/Apps/$NAME.c" ]; then
        SRC="Binaries/Apps/$NAME.c"
    elif [ -f "Binaries/$NAME.c" ]; then
        SRC="Binaries/$NAME.c"
    else
        echo "Error: Could not find source for $NAME"
        exit 1
    fi

    cl65 -O -Oi -Or --static-locals --add-source --cpu 65C02 -t none -S \
        -I "Binaries" \
        -o "Binaries/build/${NAME}_app.s" "$SRC"

    python3 Linker/generate_app_cfg.py > "Binaries/build/app.cfg"

    # Assemble and link without default libraries to avoid crt0 conflict
    ca65 --cpu 65C02 -g -o "Binaries/build/App-Runtime.o" Linker/App-Runtime.s
    ca65 --cpu 65C02 -g -o "Binaries/build/${NAME}_app.o" "Binaries/build/${NAME}_app.s"
    
    ld65 -C "Binaries/build/app.cfg" \
        -o "Binaries/build/${NAME}.raw" \
        -m "Binaries/build/${NAME}.map" \
        "Binaries/build/App-Runtime.o" \
        "Binaries/build/${NAME}_app.o" \
        none.lib

    # Prepend 4-byte header: magic $55 $44, entry offset $00 $00
    mkdir -p output/apps
    printf '\x55\x44\x00\x00' > "output/apps/${NAME}.app"
    cat "Binaries/build/${NAME}.raw" >> "output/apps/${NAME}.app"
    echo "App saved to output/apps/${NAME}.app"
    exit 0
fi

if [ "$NAME" == "all" ]; then
    echo "--- Compiling ALL Targets ---"
    
    $0 eater

    for f in Binaries/*.s; do
        if [ -f "$f" ]; then
            $0 $(basename "${f%.s}")
        fi
    done

    for f in Binaries/*.c; do
        if [ -f "$f" ]; then
            $0 $(basename "${f%.c}")
        fi
    done
    exit 0
fi

mkdir -p Binaries/build
mkdir -p output

if [ "$NAME" == "eater" ]; then
    echo "--- Compiling BASIC ---"
    (cd Linker/msbasic && chmod +x make.sh && ./make.sh) > /dev/null 2>&1
    cp Linker/msbasic/tmp/eater.bin Binaries/build/eater.bin
    mkdir -p output/rom
    cp Linker/msbasic/tmp/eater.bin output/rom/eater.bin

elif [ -f "SID/generator/build/$NAME.s" ]; then
    echo "--- Assembling $NAME.s (ASM from SID/generator/build/) ---"
    mkdir -p Binaries/build
    ca65 --cpu 65C02 -g -l "Binaries/build/$NAME.lst" -o "Binaries/build/$NAME.o" "SID/generator/build/$NAME.s"
    ld65 -C Linker/raw.cfg -m "Binaries/build/$NAME.map" -vm -Ln "Binaries/build/$NAME.lbl" --dbgfile "Binaries/build/$NAME.dbg" -o "Binaries/build/$NAME.bin" "Binaries/build/$NAME.o"
    mkdir -p output/rom
    cp "Binaries/build/$NAME.bin" "output/rom/$NAME.bin"

elif [ -f "Binaries/$NAME.s" ]; then
    echo "--- Assembling $NAME.s (ASM) ---"
    ca65 --cpu 65C02 -g -l "Binaries/build/$NAME.lst" -o "Binaries/build/$NAME.o" "Binaries/$NAME.s"
    ld65 -C Linker/raw.cfg -m "Binaries/build/$NAME.map" -vm -Ln "Binaries/build/$NAME.lbl" --dbgfile "Binaries/build/$NAME.dbg" -o "Binaries/build/$NAME.bin" "Binaries/build/$NAME.o"
    mkdir -p output/rom
    cp "Binaries/build/$NAME.bin" "output/rom/$NAME.bin"

elif [ "$NAME" == "microDOS" ]; then
    echo "--- Compiling microDOS (Multi-module C) ---"

    echo "  Compilando módulos en microDOS/..."
    MICRODOS_OBJS=""
    for f in Binaries/microDOS/*.c; do
        if [ -f "$f" ]; then
            mod_name=$(basename "${f%.c}")
            cl65 -O -Oi -Or --static-locals --add-source --cpu 65C02 -t none -S \
                -I "Binaries" -I "Binaries/microDOS" \
                -o "Binaries/build/$mod_name.s" "$f"
            MICRODOS_OBJS="$MICRODOS_OBJS Binaries/build/$mod_name.s"
        fi
    done

    echo "  Compilando main..."
    cl65 -O -Oi -Or --static-locals --add-source --cpu 65C02 -t none -S \
        -I "Binaries" -I "Binaries/microDOS" \
        -o "Binaries/build/microDOS.s" "Binaries/microDOS.c"

    # Detect features and accumulate flags (excluding GPU)
    CFG_FLAGS=""
    if grep -r -q '#include "Libs/NET.h"' Binaries/microDOS/ Binaries/microDOS.c; then
        echo "  [NET detected] Added --net to Linker"
        CFG_FLAGS="$CFG_FLAGS --net"
    fi
    CFG_FLAGS="$CFG_FLAGS --microDOS"

    echo "  [SD.h detected] Compilando FatFs (ff.c + diskio.c)..."
    cl65 -O --cpu 65C02 -t none -S -I "Binaries" -o Binaries/build/ff.s Binaries/Libs/fatfs/ff.c
    cl65 -O --cpu 65C02 -t none -S -I "Binaries" -o Binaries/build/diskio.s Binaries/Libs/fatfs/diskio.c
    cl65 -O --cpu 65C02 -t none -S -I "Binaries" -o Binaries/build/bios_utils.s Binaries/Libs/BIOS.c
    cl65 -O --cpu 65C02 -t none -S -I "Binaries" -o Binaries/build/net_utils.s Binaries/Libs/NET.c
    cl65 -O --cpu 65C02 -t none -S -I "Binaries" -o Binaries/build/sd.s Binaries/Libs/SD.c
    EXTRA_OBJS="Binaries/build/bios_utils.s Binaries/build/net_utils.s Binaries/build/ff.s Binaries/build/diskio.s Binaries/build/sd.s"

    echo "  Generating dynamic Linker CFG..."
    python3 Linker/generate_cfg.py $CFG_FLAGS > "Binaries/build/C-Runtime-dynamic.cfg"
    LINKER_CFG="Binaries/build/C-Runtime-dynamic.cfg"

    echo "  Enlazando binario final..."
    cl65 -g --cpu 65C02 -t none -C "$LINKER_CFG" \
        -o "Binaries/build/microDOS.bin" \
        -m "Binaries/build/microDOS.map" -vm \
        -l "Binaries/build/microDOS.lst" \
        -Wl -Ln,"Binaries/build/microDOS.lbl" \
        -Wl --dbgfile,"Binaries/build/microDOS.dbg" \
        Linker/microdos_bios.s Linker/C-Runtime.s \
        "Binaries/build/microDOS.s" \
        $MICRODOS_OBJS \
        $EXTRA_OBJS

    mkdir -p output/rom
    cp "Binaries/build/microDOS.bin" "output/rom/microDOS.bin"
    
elif [ -f "Binaries/$NAME.c" ]; then
    echo "--- Compiling $NAME.c (C) ---"
    cl65 -O -Oi -Or --static-locals --add-source --cpu 65C02 -t none -S \
        -o "Binaries/build/$NAME.s" "Binaries/$NAME.c"

    # Detect features and accumulate flags
    CFG_FLAGS=""
    if grep -q '#include "Libs/GPUDoubleBuffer.h"' "Binaries/$NAME.c"; then
        echo "  [GPUDoubleBuffer detected] Added --double-buffer to Linker"
        CFG_FLAGS="$CFG_FLAGS --double-buffer"
    elif grep -q '#include "Libs/GPU.h"' "Binaries/$NAME.c"; then
        echo "  [GPU detected] Added --gpu to Linker"
        CFG_FLAGS="$CFG_FLAGS --gpu"
    fi
    
    if grep -q '#include "Libs/NET.h"' "Binaries/$NAME.c"; then
        echo "  [NET detected] Added --net to Linker"
        CFG_FLAGS="$CFG_FLAGS --net"
    fi

    echo "  Generating dynamic Linker CFG..."
    python3 Linker/generate_cfg.py $CFG_FLAGS > "Binaries/build/C-Runtime-dynamic.cfg"
    LINKER_CFG="Binaries/build/C-Runtime-dynamic.cfg"

    # FatFs: if the program uses SD.h, compile ff.c and diskio.c
    cl65 -O --cpu 65C02 -t none -S -I "Binaries" -o Binaries/build/bios_utils.s Binaries/Libs/BIOS.c
    EXTRA_OBJS="Binaries/build/bios_utils.s"
    if grep -q '#include "Libs/SD.h"' "Binaries/$NAME.c"; then
        echo "  [SD.h detected] Compilando FatFs (ff.c + diskio.c)..."
        cl65 -O --cpu 65C02 -t none -S \
            -o Binaries/build/ff.s Binaries/Libs/fatfs/ff.c
        cl65 -O --cpu 65C02 -t none -S \
            -o Binaries/build/diskio.s Binaries/Libs/fatfs/diskio.c
        cl65 -O --cpu 65C02 -t none -S \
            -o Binaries/build/sd.s Binaries/Libs/SD.c
        EXTRA_OBJS="$EXTRA_OBJS Binaries/build/ff.s Binaries/build/diskio.s Binaries/build/sd.s"
    fi

    cl65 -g --cpu 65C02 -t none -C "$LINKER_CFG" \
        -o "Binaries/build/$NAME.bin" \
        -m "Binaries/build/$NAME.map" -vm \
        -l "Binaries/build/$NAME.lst" \
        -Wl -Ln,"Binaries/build/$NAME.lbl" \
        -Wl --dbgfile,"Binaries/build/$NAME.dbg" \
        Linker/bios.s Linker/C-Runtime.s "Binaries/build/$NAME.s" \
        $EXTRA_OBJS
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