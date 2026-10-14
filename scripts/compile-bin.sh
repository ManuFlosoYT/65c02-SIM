#!/bin/bash
set -e

# LLVM-MOS Toolchain Detection
if [ -n "$MOS_CLANG" ] && [ -x "$MOS_CLANG" ]; then
    :
elif [ -x "build/llvm_mos/llvm-mos/bin/mos-common-clang" ]; then
    MOS_CLANG="$(pwd)/build/llvm_mos/llvm-mos/bin/mos-common-clang"
elif [ -x "output/toolchain/bin/mos-common-clang" ]; then
    MOS_CLANG="$(pwd)/output/toolchain/bin/mos-common-clang"
elif command -v mos-common-clang >/dev/null 2>&1; then
    MOS_CLANG="$(command -v mos-common-clang)"
elif command -v mos-clang >/dev/null 2>&1; then
    MOS_CLANG="$(command -v mos-clang)"
else
    echo "Error: LLVM-MOS compiler (mos-common-clang or mos-clang) not found."
    exit 1
fi

compile_with_opt_fallback() {
    local out="$1"
    local ld_script="$2"
    shift 2
    local sources=("$@")

    local last_err=""
    for opt in -O3 -O2 -Os -Oz; do
        echo "  [LLVM-MOS] Compiling with $opt..."
        local err_log
        err_log=$(mktemp)
        if "$MOS_CLANG" -mcpu=mosw65c02 $opt -fnonreentrant -ffunction-sections -fdata-sections -Wl,--gc-sections -T "$ld_script" "${sources[@]}" -o "$out" > "$err_log" 2>&1; then
            cat "$err_log"
            rm -f "$err_log"
            echo "  [LLVM-MOS] Successfully compiled with $opt."
            return 0
        fi

        last_err=$(cat "$err_log")
        rm -f "$err_log"

        if echo "$last_err" | grep -E -q "will not fit in region|overflowed by|address range overlaps with"; then
            echo "  [LLVM-MOS] $opt binary too large (memory overflow), falling back..."
        else
            echo "  [LLVM-MOS] Compilation error at $opt (not a size issue):"
            echo "$last_err"
            return 1
        fi
    done

    echo "Error: Binary too large for all optimization levels (-O3, -O2, -Os, -Oz)."
    echo "$last_err"
    return 1
}

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

MULTITHREAD=false
for arg in "$@"; do
    if [ "$arg" == "-multithread" ] || [ "$arg" == "--multithread" ]; then
        MULTITHREAD=true
    fi
done

mkdir -p sdk/src/build
mkdir -p output

# --microDOS: compile a .c file as a microDOS .app binary
if [ "$2" == "--microDOS" ] || [ "$3" == "--microDOS" ]; then
    echo "--- Compiling $NAME.app (microDOS App with LLVM-MOS) ---"
    SRC=""
    if [ -f "sdk/microdos/Apps/$NAME.c" ]; then
        SRC="sdk/microdos/Apps/$NAME.c"
    elif [ -f "sdk/src/$NAME.c" ]; then
        SRC="sdk/src/$NAME.c"
    else
        echo "Error: Could not find source for $NAME"
        exit 1
    fi

    APP_LD="sdk/src/build/${NAME}_app.ld"
    python3 tools/linker/generate_app_link_ld.py > "$APP_LD"

    APP_RAW="sdk/src/build/${NAME}.raw"
    compile_with_opt_fallback "$APP_RAW" "$APP_LD" \
        -I "sdk/src" -I "sdk/microdos" \
        sdk/linker/App-Runtime.s "$SRC"

    mkdir -p output/apps
    cp "$APP_RAW" "output/apps/${NAME}.app"
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

if [ "$NAME" == "eater" ]; then
    echo "--- Compiling BASIC ---"
    if [ -f "sdk/msbasic/make.sh" ] && command -v ca65 >/dev/null 2>&1; then
        (cd sdk/msbasic && chmod +x make.sh && ./make.sh) > /dev/null 2>&1 || true
    fi
    if [ -f "sdk/msbasic/tmp/eater.bin" ]; then
        cp sdk/msbasic/tmp/eater.bin sdk/src/build/eater.bin
        mkdir -p output/rom
        cp sdk/msbasic/tmp/eater.bin output/rom/eater.bin
    elif [ -f "output/rom/eater.bin" ]; then
        cp output/rom/eater.bin sdk/src/build/eater.bin
    fi
    exit 0

elif [ -f "tools/sid/build/$NAME.s" ]; then
    echo "--- Assembling $NAME.s (ASM from tools/sid/build/) ---"
    "$MOS_CLANG" -mcpu=mosw65c02 -nostartfiles -T sdk/linker/link.ld -o "sdk/src/build/$NAME.bin" "tools/sid/build/$NAME.s"
    mkdir -p output/rom
    cp "sdk/src/build/$NAME.bin" "output/rom/$NAME.bin"

elif [ -f "sdk/src/$NAME.s" ]; then
    echo "--- Assembling $NAME.s (ASM with LLVM-MOS) ---"
    "$MOS_CLANG" -mcpu=mosw65c02 -nostartfiles -T sdk/linker/link.ld -o "sdk/src/build/$NAME.bin" "sdk/src/$NAME.s"
    mkdir -p output/rom
    cp "sdk/src/build/$NAME.bin" "output/rom/$NAME.bin"

elif [ "$NAME" == "microDOS" ]; then
    echo "--- Compiling microDOS (Multi-module C with LLVM-MOS) ---"

    CFG_FLAGS=""
    if grep -r -q '#include "Libs/NET.h"' sdk/microdos/src/ sdk/microdos/src/microDOS.c; then
        echo "  [NET detected] Added --net to Linker"
        CFG_FLAGS="$CFG_FLAGS --net"
    fi
    if grep -r -q '#include ".*SID.h"' sdk/microdos/src/microDOS/; then
        echo "  [SID detected] Added --sid to Linker"
        CFG_FLAGS="$CFG_FLAGS --sid"
    fi
    CFG_FLAGS="$CFG_FLAGS --microDOS"

    LINKER_LD="sdk/src/build/microDOS-dynamic.ld"
    python3 tools/linker/generate_link_ld.py $CFG_FLAGS > "$LINKER_LD"

    MICRODOS_SRCS=(
        sdk/linker/microdos_bios.s
        sdk/microdos/src/microDOS.c
        sdk/microdos/src/microDOS/*.c
        sdk/src/Libs/BIOS.c
        sdk/src/Libs/NET.c
        sdk/src/Libs/SD.c
        sdk/src/Libs/fatfs/ff.c
        sdk/src/Libs/fatfs/diskio.c
    )

    compile_with_opt_fallback "sdk/src/build/microDOS.bin" "$LINKER_LD" \
        -mlto-zp=200 \
        -I "sdk/src" -I "sdk/microdos/src" -D ENABLE_SD_SUPPORT=1 \
        "${MICRODOS_SRCS[@]}"

    mkdir -p output/rom
    cp "sdk/src/build/microDOS.bin" "output/rom/microDOS.bin"

elif [ -f "sdk/src/$NAME.c" ]; then
    echo "--- Compiling $NAME.c (C with LLVM-MOS) ---"

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

    LINKER_LD="sdk/src/build/C-Runtime-dynamic.ld"
    python3 tools/linker/generate_link_ld.py $CFG_FLAGS > "$LINKER_LD"

    EXTRA_SRCS=("sdk/linker/bios.s" "sdk/src/Libs/BIOS.c")
    EXTRA_FLAGS=("-I" "sdk/src")

    if grep -q '#include "Libs/SD.h"' "sdk/src/$NAME.c"; then
        echo "  [SD.h detected] Adding FatFs (ff.c + diskio.c + sd.c)..."
        EXTRA_SRCS+=("sdk/src/Libs/fatfs/ff.c" "sdk/src/Libs/fatfs/diskio.c" "sdk/src/Libs/SD.c")
        EXTRA_FLAGS+=("-D" "ENABLE_SD_SUPPORT=1")
    fi

    compile_with_opt_fallback "sdk/src/build/$NAME.bin" "$LINKER_LD" \
        "${EXTRA_FLAGS[@]}" "sdk/src/$NAME.c" "${EXTRA_SRCS[@]}"

    mkdir -p output/rom
    cp "sdk/src/build/$NAME.bin" "output/rom/$NAME.bin"

else
    echo "Error: sdk/src/$NAME.c or sdk/src/$NAME.s does not exist"
    list_programs
    exit 1
fi

echo "Compilation finished. Binary saved to output/rom/$NAME.bin"
exit 0
