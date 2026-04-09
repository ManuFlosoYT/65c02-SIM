#!/bin/bash
set -e

list_programs() {
    echo "Available programs:"
    echo "  - all (Compile all programs)"
    echo "  - eater (WOZMON / BASIC)"
    for f in sdk/src/*.c; do
        [ -f "$f" ] && echo "  - $(basename "${f%.c}") (C)"
    done
    for f in sdk/src/*.s; do
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
    if [ -f "sdk/microdosapps/Apps/$NAME.c" ]; then
        SRC="sdk/microdosapps/Apps/$NAME.c"
    elif [ -f "sdk/src/$NAME.c" ]; then
        SRC="sdk/src/$NAME.c"
    else
        echo "Error: Could not find source for $NAME"
        exit 1
    fi

    cl65 -O -Oi -Or --static-locals --add-source --cpu 65C02 -t none -S \
        -I "sdk/src" -I "sdk/microdosapps" \
        -o "sdk/src/build/${NAME}_app.s" "$SRC"

    python3 tools/linker/generate_app_cfg.py > "sdk/src/build/app.cfg"

    ca65 --cpu 65C02 -g -o "sdk/src/build/App-Runtime.o" sdk/linker/App-Runtime.s
    ca65 --cpu 65C02 -g -o "sdk/src/build/${NAME}_app.o" "sdk/src/build/${NAME}_app.s"
    
    ld65 -C "sdk/src/build/app.cfg" \
        -o "sdk/src/build/${NAME}.raw" \
        -m "sdk/src/build/${NAME}.map" \
        "sdk/src/build/App-Runtime.o" \
        "sdk/src/build/${NAME}_app.o" \
        none.lib

    # El Linker ya incluye la cabecera uDOS generada en .segment "HEADER"
    mkdir -p output/apps
    cp "sdk/src/build/${NAME}.raw" "output/apps/${NAME}.app"
    echo "App saved to output/apps/${NAME}.app"
    exit 0
fi

if [ "$NAME" == "all" ]; then
    echo "--- Compiling ALL Targets ---"
    
    $0 eater

    for f in sdk/src/*.s; do
        if [ -f "$f" ]; then
            $0 $(basename "${f%.s}")
        fi
    done

    for f in sdk/src/*.c; do
        if [ -f "$f" ]; then
            $0 $(basename "${f%.c}")
        fi
    done
    exit 0
fi

mkdir -p sdk/src/build
mkdir -p output

if [ "$NAME" == "eater" ]; then
    echo "--- Compiling BASIC ---"
    (cd sdk/msbasic && chmod +x make.sh && ./make.sh) > /dev/null 2>&1
    cp sdk/msbasic/tmp/eater.bin sdk/src/build/eater.bin
    mkdir -p output/rom
    cp sdk/msbasic/tmp/eater.bin output/rom/eater.bin

elif [ -f "tools/sid/build/$NAME.s" ]; then
    echo "--- Assembling $NAME.s (ASM from tools/sid/build/) ---"
    mkdir -p sdk/src/build
    ca65 --cpu 65C02 -g -l "sdk/src/build/$NAME.lst" -o "sdk/src/build/$NAME.o" "tools/sid/build/$NAME.s"
    ld65 -C sdk/linker/raw.cfg -m "sdk/src/build/$NAME.map" -vm -Ln "sdk/src/build/$NAME.lbl" --dbgfile "sdk/src/build/$NAME.dbg" -o "sdk/src/build/$NAME.bin" "sdk/src/build/$NAME.o"
    mkdir -p output/rom
    cp "sdk/src/build/$NAME.bin" "output/rom/$NAME.bin"

elif [ -f "sdk/src/$NAME.s" ]; then
    echo "--- Assembling $NAME.s (ASM) ---"
    ca65 --cpu 65C02 -g -l "sdk/src/build/$NAME.lst" -o "sdk/src/build/$NAME.o" "sdk/src/$NAME.s"
    ld65 -C sdk/linker/raw.cfg -m "sdk/src/build/$NAME.map" -vm -Ln "sdk/src/build/$NAME.lbl" --dbgfile "sdk/src/build/$NAME.dbg" -o "sdk/src/build/$NAME.bin" "sdk/src/build/$NAME.o"
    mkdir -p output/rom
    cp "sdk/src/build/$NAME.bin" "output/rom/$NAME.bin"

elif [ "$NAME" == "microDOS" ]; then
    echo "--- Compiling microDOS (Multi-module C) ---"

    echo "  Compilando módulos en microDOS/..."
    MICRODOS_OBJS=""
    for f in sdk/src/microDOS/*.c; do
        if [ -f "$f" ]; then
            mod_name=$(basename "${f%.c}")
            cl65 -O -Oi -Or --static-locals --add-source --cpu 65C02 -t none -S \
                -I "sdk/src" -I "sdk/src/microDOS" \
                -o "sdk/src/build/$mod_name.s" "$f"
            MICRODOS_OBJS="$MICRODOS_OBJS sdk/src/build/$mod_name.s"
        fi
    done

    echo "  Compilando main..."
    cl65 -O -Oi -Or --static-locals --add-source --cpu 65C02 -t none -S \
        -I "sdk/src" -I "sdk/src/microDOS" \
        -o "sdk/src/build/microDOS.s" "sdk/src/microDOS.c"

    CFG_FLAGS=""
    if grep -r -q '#include "Libs/NET.h"' sdk/src/microDOS/ sdk/src/microDOS.c; then
        echo "  [NET detected] Added --net to Linker"
        CFG_FLAGS="$CFG_FLAGS --net"
    fi
    if grep -r -q '#include ".*SID.h"' sdk/src/microDOS/; then
        echo "  [SID detected] Added --sid to Linker"
        CFG_FLAGS="$CFG_FLAGS --sid"
    fi
    CFG_FLAGS="$CFG_FLAGS --microDOS"

    echo "  [SD.h detected] Compilando FatFs (ff.c + diskio.c)..."
    cl65 -O --cpu 65C02 -t none -S -I "sdk/src" -o sdk/src/build/ff.s sdk/src/Libs/fatfs/ff.c
    cl65 -O --cpu 65C02 -t none -S -I "sdk/src" -o sdk/src/build/diskio.s sdk/src/Libs/fatfs/diskio.c
    cl65 -O --cpu 65C02 -t none -S -I "sdk/src" -o sdk/src/build/bios_utils.s sdk/src/Libs/BIOS.c
    cl65 -O --cpu 65C02 -t none -S -I "sdk/src" -o sdk/src/build/net_utils.s sdk/src/Libs/NET.c
    cl65 -O --cpu 65C02 -t none -S -I "sdk/src" -o sdk/src/build/sd.s sdk/src/Libs/SD.c
    EXTRA_OBJS="sdk/src/build/bios_utils.s sdk/src/build/net_utils.s sdk/src/build/ff.s sdk/src/build/diskio.s sdk/src/build/sd.s"

    echo "  Generating dynamic Linker CFG..."
    python3 tools/linker/generate_cfg.py $CFG_FLAGS > "sdk/src/build/C-Runtime-dynamic.cfg"
    LINKER_CFG="sdk/src/build/C-Runtime-dynamic.cfg"

    echo "  Enlazando binario final..."
    cl65 -g --cpu 65C02 -t none -C "$LINKER_CFG" \
        -o "sdk/src/build/microDOS.bin" \
        -m "sdk/src/build/microDOS.map" -vm \
        -l "sdk/src/build/microDOS.lst" \
        -Wl -Ln,"sdk/src/build/microDOS.lbl" \
        -Wl --dbgfile,"sdk/src/build/microDOS.dbg" \
        sdk/linker/microdos_bios.s sdk/linker/C-Runtime.s \
        "sdk/src/build/microDOS.s" \
        $MICRODOS_OBJS \
        $EXTRA_OBJS

    mkdir -p output/rom
    cp "sdk/src/build/microDOS.bin" "output/rom/microDOS.bin"
    
elif [ -f "sdk/src/$NAME.c" ]; then
    echo "--- Compiling $NAME.c (C) ---"
    cl65 -O -Oi -Or --static-locals --add-source --cpu 65C02 -t none -S \
        -o "sdk/src/build/$NAME.s" "sdk/src/$NAME.c"

    CFG_FLAGS=""
    if grep -q '#include "Libs/GPUDoubleBuffer.h"' "sdk/src/$NAME.c"; then
        echo "  [GPUDoubleBuffer detected] Added --double-buffer to Linker"
        CFG_FLAGS="$CFG_FLAGS --double-buffer"
    elif grep -q '#include "Libs/GPU.h"' "sdk/src/$NAME.c"; then
        echo "  [GPU detected] Added --gpu to Linker"
        CFG_FLAGS="$CFG_FLAGS --gpu"
    fi
    
    if grep -q '#include "Libs/NET.h"' "sdk/src/$NAME.c"; then
        echo "  [NET detected] Added --net to Linker"
        CFG_FLAGS="$CFG_FLAGS --net"
    fi
    if grep -q '#include "Libs/SID.h"' "sdk/src/$NAME.c"; then
        echo "  [SID detected] Added --sid to Linker"
        CFG_FLAGS="$CFG_FLAGS --sid"
    fi

    echo "  Generating dynamic Linker CFG..."
    python3 tools/linker/generate_cfg.py $CFG_FLAGS > "sdk/src/build/C-Runtime-dynamic.cfg"
    LINKER_CFG="sdk/src/build/C-Runtime-dynamic.cfg"

    cl65 -O --cpu 65C02 -t none -S -I "sdk/src" -o sdk/src/build/bios_utils.s sdk/src/Libs/BIOS.c
    EXTRA_OBJS="sdk/src/build/bios_utils.s"
    if grep -q '#include "Libs/SD.h"' "sdk/src/$NAME.c"; then
        echo "  [SD.h detected] Compilando FatFs (ff.c + diskio.c)..."
        cl65 -O --cpu 65C02 -t none -S \
            -o sdk/src/build/ff.s sdk/src/Libs/fatfs/ff.c
        cl65 -O --cpu 65C02 -t none -S \
            -o sdk/src/build/diskio.s sdk/src/Libs/fatfs/diskio.c
        cl65 -O --cpu 65C02 -t none -S \
            -o sdk/src/build/sd.s sdk/src/Libs/SD.c
        EXTRA_OBJS="$EXTRA_OBJS sdk/src/build/ff.s sdk/src/build/diskio.s sdk/src/build/sd.s"
    fi

    cl65 -g --cpu 65C02 -t none -C "$LINKER_CFG" \
        -o "sdk/src/build/$NAME.bin" \
        -m "sdk/src/build/$NAME.map" -vm \
        -l "sdk/src/build/$NAME.lst" \
        -Wl -Ln,"sdk/src/build/$NAME.lbl" \
        -Wl --dbgfile,"sdk/src/build/$NAME.dbg" \
        sdk/linker/bios.s sdk/linker/C-Runtime.s "sdk/src/build/$NAME.s" \
        $EXTRA_OBJS
    mv "sdk/src/$NAME.o" "sdk/src/build/$NAME.o" 2>/dev/null || true
    mkdir -p output/rom
    cp "sdk/src/build/$NAME.bin" "output/rom/$NAME.bin"

else
    echo "Error: sdk/src/$NAME.c or sdk/src/$NAME.s does not exist"
    list_programs
    exit 1
fi

echo "Compilation finished. Binary saved to output/rom/$NAME.bin"
exit 0