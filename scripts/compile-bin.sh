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

MOS_BIN_DIR="$(dirname "$MOS_CLANG")"
LLVM_OBJCOPY="$MOS_BIN_DIR/llvm-objcopy"
if [ ! -x "$LLVM_OBJCOPY" ]; then
    if command -v llvm-objcopy >/dev/null 2>&1; then
        LLVM_OBJCOPY="$(command -v llvm-objcopy)"
    else
        LLVM_OBJCOPY="llvm-objcopy"
    fi
fi

LLVM_OBJDUMP="$MOS_BIN_DIR/llvm-objdump"
if [ ! -x "$LLVM_OBJDUMP" ]; then
    if command -v llvm-objdump >/dev/null 2>&1; then
        LLVM_OBJDUMP="$(command -v llvm-objdump)"
    else
        LLVM_OBJDUMP="llvm-objdump"
    fi
fi

compile_with_opt_fallback() {
    local out="$1"
    local ld_script="$2"
    shift 2
    local sources=("$@")

    local last_err=""
    for gflag in "-g" ""; do
        for opt in -Os -Oz; do
            echo "  [LLVM-MOS] Compiling with $opt $gflag..."
            local err_log
            err_log=$(mktemp)

            local opt_flags=()

            if "$MOS_CLANG" -mcpu=mosw65c02 $opt $gflag -flto -ffunction-sections -fdata-sections -fno-unroll-loops -fnonreentrant -Wl,--gc-sections -Wl,--lto-O2 "${opt_flags[@]}" -T "$ld_script" "${sources[@]}" -o "$out" > "$err_log" 2>&1; then
                cat "$err_log"
                rm -f "$err_log"
                echo "  [LLVM-MOS] Successfully compiled with $opt $gflag."
                return 0
            fi

            last_err=$(cat "$err_log")
            rm -f "$err_log"

            if echo "$last_err" | grep -E -q "will not fit in region|overflowed by|address range overlaps with"; then
                echo "  [LLVM-MOS] $opt $gflag binary too large (memory overflow):"
                echo "  [LLVM-MOS] Falling back to next optimization level..."
            else
                echo "  [LLVM-MOS] Compilation error at $opt $gflag (not a size issue):"
                echo "$last_err"
                return 1
            fi
        done
    done

    echo "Error: Binary too large for all optimization levels (-Os, -Oz)."
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
    echo "Usage: $0 <program_name> [--microDOS] [--unity|--no-unity] [-multithread]"
    list_programs
    exit 1
fi

NAME=$1

MULTITHREAD=false
UNITY=false
NO_UNITY=false
for arg in "$@"; do
    if [ "$arg" == "-multithread" ] || [ "$arg" == "--multithread" ]; then
        MULTITHREAD=true
    elif [ "$arg" == "-unity" ] || [ "$arg" == "--unity" ]; then
        UNITY=true
    elif [ "$arg" == "-no-unity" ] || [ "$arg" == "--no-unity" ]; then
        NO_UNITY=true
    fi
done

mkdir -p sdk/src/build
mkdir -p output

# --microDOS: compile a .c file as a microDOS .app binary
if [ "$2" == "--microDOS" ] || [ "$3" == "--microDOS" ] || [ "$4" == "--microDOS" ]; then
    echo "--- Compiling $NAME.app (microDOS App with LLVM-MOS -Os) ---"
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
    APP_FLAGS=""
    if grep -q "Libs/GPU.h" "$SRC" || grep -q "GPUDoubleBuffer.h" "$SRC"; then
        APP_FLAGS="--gpu"
    fi

    NUM_OVERLAYS=0
    prev_arg=""
    for arg in "$@"; do
        if [ "$prev_arg" == "--overlays" ] && [[ "$arg" =~ ^[0-9]+$ ]]; then
            NUM_OVERLAYS="$arg"
        elif [[ "$arg" =~ ^--overlays=([0-9]+)$ ]]; then
            NUM_OVERLAYS="${BASH_REMATCH[1]}"
        fi
        prev_arg="$arg"
    done
    if [ "$NUM_OVERLAYS" -eq 0 ]; then
        MAX_OVERLAY=$(grep -ho -E '\.overlay_?([0-9]+)' "$SRC" 2>/dev/null | sed -E 's/\.overlay_?//' | sort -n | tail -n 1 || true)
        if [ -n "$MAX_OVERLAY" ] && [ "$MAX_OVERLAY" -gt 0 ]; then
            NUM_OVERLAYS="$MAX_OVERLAY"
        fi
    fi

    if [ "$NUM_OVERLAYS" -gt 0 ]; then
        APP_FLAGS="$APP_FLAGS --overlays $NUM_OVERLAYS"
        echo "  [Overlays configured] Using $NUM_OVERLAYS overlay pages for $NAME.app"
    fi

    python3 tools/linker/generate_app_link_ld.py $APP_FLAGS > "$APP_LD"

    APP_RAW="sdk/src/build/${NAME}.raw"
    compile_with_opt_fallback "$APP_RAW" "$APP_LD" \
        -std=c23 -D MICRODOS=1 \
        -I "sdk/src" -I "sdk/microdos" \
        -linit-stack -lcopy-data -lzero-bss \
        sdk/linker/App-Runtime.s "$SRC"

    mkdir -p output/apps
    APP_OUT="output/apps/${NAME}.app"

    ELF_FILE="${APP_RAW}.elf"
    if [ ! -f "$ELF_FILE" ] && [ -f "${APP_RAW%.raw}.elf" ]; then
        ELF_FILE="${APP_RAW%.raw}.elf"
    fi

    OVERLAY_SECTIONS=()
    if [ -f "$ELF_FILE" ]; then
        while IFS= read -r sec; do
            [ -n "$sec" ] && OVERLAY_SECTIONS+=("$sec")
        done < <("$LLVM_OBJDUMP" -h "$ELF_FILE" 2>/dev/null | awk '$2 ~ /^\.overlay_/ && $3 != "00000000" { print $2 }' | sort -V || true)
    fi

    if [ ${#OVERLAY_SECTIONS[@]} -eq 0 ]; then
        cp "$APP_RAW" "$APP_OUT"
        echo "App saved to $APP_OUT"
    else
        echo "  [Overlay packaging] Processing ${#OVERLAY_SECTIONS[@]} overlay sections: ${OVERLAY_SECTIONS[*]}"

        RAW_SIZE=$(wc -c < "$APP_RAW" | tr -d ' ')
        EXPECTED_OVERLAY_BYTES=$((${#OVERLAY_SECTIONS[@]} * 8192))
        HEADER_CORE_SIZE=$(python3 -c "import struct, sys
try:
    with open(sys.argv[1], 'rb') as f:
        hdr = f.read(16)
        print(struct.unpack('<4sBBHHHHH', hdr)[4])
except Exception:
    print(0)
" "$APP_RAW" 2>/dev/null || echo 0)

        if [ "$RAW_SIZE" -ge $((HEADER_CORE_SIZE + EXPECTED_OVERLAY_BYTES)) ] && [ "$HEADER_CORE_SIZE" -gt 0 ]; then
            echo "  [Overlay packaging] Binary already contains fully padded overlay pages from linker OUTPUT_FORMAT."
            cp "$APP_RAW" "$APP_OUT"
            CORE_SIZE=$HEADER_CORE_SIZE
        else
            echo "  [Overlay packaging] Extracting and padding overlay sections via llvm-objcopy..."
            cp "$APP_RAW" "$APP_OUT"
            CORE_SIZE=$(wc -c < "$APP_RAW" | tr -d ' ')

            PAGE_IDX=0
            for sec in "${OVERLAY_SECTIONS[@]}"; do
                TMP_PAGE="sdk/src/build/${NAME}_${sec}.bin"
                rm -f "$TMP_PAGE"
                "$LLVM_OBJCOPY" --dump-section="${sec}=${TMP_PAGE}" "$ELF_FILE"

                SEC_SIZE=$(wc -c < "$TMP_PAGE" | tr -d ' ')
                if [ "$SEC_SIZE" -gt 8192 ]; then
                    echo "Error: Overlay section $sec ($SEC_SIZE bytes) exceeds 8192 bytes (0x2000) window capacity!"
                    rm -f "$TMP_PAGE"
                    exit 1
                fi

                truncate -s 8192 "$TMP_PAGE"
                cat "$TMP_PAGE" >> "$APP_OUT"
                rm -f "$TMP_PAGE"
                echo "  [Overlay Page $PAGE_IDX] Section $sec ($SEC_SIZE bytes -> padded to 8192 bytes)"
                PAGE_IDX=$((PAGE_IDX + 1))
            done
        fi

        NUM_PAGES=${#OVERLAY_SECTIONS[@]}
        python3 -c "
import struct, sys
app_path = sys.argv[1]
core_size = int(sys.argv[2])
num_pages = int(sys.argv[3])
with open(app_path, 'r+b') as f:
    f.seek(8)
    f.write(struct.pack('<H', core_size))
    f.seek(14)
    val = struct.unpack('<H', f.read(2))[0]
    if val < 0x5FC0:
        f.seek(14)
        f.write(struct.pack('<H', num_pages))
" "$APP_OUT" "$CORE_SIZE" "$NUM_PAGES"
        TOTAL_SIZE=$(wc -c < "$APP_OUT" | tr -d ' ')
        echo "App with $NUM_PAGES overlay pages successfully packaged to $APP_OUT (core: $CORE_SIZE bytes, total: $TOTAL_SIZE bytes)"
    fi
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
    if [ -f "sdk/msbasic/tmp/eater.dbg" ]; then
        mkdir -p output/rom
        cp sdk/msbasic/tmp/eater.dbg output/rom/eater.dbg
    fi
    exit 0

elif [ -f "tools/sid/build/$NAME.s" ]; then
    echo "--- Assembling $NAME.s (ASM from tools/sid/build/) ---"
    "$MOS_CLANG" -mcpu=mosw65c02 -g -nostartfiles -T sdk/linker/link.ld -o "sdk/src/build/$NAME.bin" "tools/sid/build/$NAME.s"
    mkdir -p output/rom
    cp "sdk/src/build/$NAME.bin" "output/rom/$NAME.bin"
    if [ -f "sdk/src/build/$NAME.bin.elf" ]; then
        cp "sdk/src/build/$NAME.bin.elf" "output/rom/$NAME.elf"
    elif [ -f "sdk/src/build/$NAME.elf" ]; then
        cp "sdk/src/build/$NAME.elf" "output/rom/$NAME.elf"
    fi

elif [ -f "sdk/src/$NAME.s" ]; then
    echo "--- Assembling $NAME.s (ASM with LLVM-MOS) ---"
    "$MOS_CLANG" -mcpu=mosw65c02 -g -nostartfiles -T sdk/linker/link.ld -o "sdk/src/build/$NAME.bin" "sdk/src/$NAME.s"
    mkdir -p output/rom
    cp "sdk/src/build/$NAME.bin" "output/rom/$NAME.bin"
    if [ -f "sdk/src/build/$NAME.bin.elf" ]; then
        cp "sdk/src/build/$NAME.bin.elf" "output/rom/$NAME.elf"
    elif [ -f "sdk/src/build/$NAME.elf" ]; then
        cp "sdk/src/build/$NAME.elf" "output/rom/$NAME.elf"
    fi

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

    USE_UNITY=false

    COMPILE_SRCS=()
    if [ "$USE_UNITY" = true ]; then
        echo "  [Unity Build] Consolidating C sources into sdk/src/build/microDOS_unity.c..."
        UNITY_C="sdk/src/build/microDOS_unity.c"
        rm -f "$UNITY_C"
        echo "/* Auto-generated Unity Build for microDOS */" > "$UNITY_C"

        for s in "${MICRODOS_SRCS[@]}"; do
            if [[ "$s" == *.c ]]; then
                echo "#include \"$PWD/$s\"" >> "$UNITY_C"
            else
                COMPILE_SRCS+=("$s")
            fi
        done
        COMPILE_SRCS+=("$UNITY_C")
    else
        COMPILE_SRCS=("${MICRODOS_SRCS[@]}")
    fi

    compile_with_opt_fallback "sdk/src/build/microDOS.bin" "$LINKER_LD" \
        -std=c23 \
        -D MICRODOS=1 \
        -mlto-zp=128 \
        -I "sdk/src" -I "sdk/microdos" -I "sdk/microdos/src" -I . -D ENABLE_SD_SUPPORT=1 \
        "${COMPILE_SRCS[@]}"

    mkdir -p output/rom
    cp "sdk/src/build/microDOS.bin" "output/rom/microDOS.bin"
    if [ -f "sdk/src/build/microDOS.bin.elf" ]; then
        cp "sdk/src/build/microDOS.bin.elf" "output/rom/microDOS.elf"
    elif [ -f "sdk/src/build/microDOS.elf" ]; then
        cp "sdk/src/build/microDOS.elf" "output/rom/microDOS.elf"
    fi

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

    COMPILE_SRCS=()
    if [ "$UNITY" = true ]; then
        echo "  [Unity Build] Consolidating C sources for $NAME..."
        UNITY_C="sdk/src/build/${NAME}_unity.c"
        rm -f "$UNITY_C"
        echo "/* Auto-generated Unity Build */" > "$UNITY_C"

        ALL_C=("sdk/src/$NAME.c")
        for s in "${EXTRA_SRCS[@]}"; do
            if [[ "$s" == *.c ]]; then
                ALL_C+=("$s")
            else
                COMPILE_SRCS+=("$s")
            fi
        done
        for c_file in "${ALL_C[@]}"; do
            echo "#include \"$PWD/$c_file\"" >> "$UNITY_C"
        done
        COMPILE_SRCS+=("$UNITY_C")
    else
        COMPILE_SRCS=("sdk/src/$NAME.c" "${EXTRA_SRCS[@]}")
    fi

    compile_with_opt_fallback "sdk/src/build/$NAME.bin" "$LINKER_LD" \
        "${EXTRA_FLAGS[@]}" "${COMPILE_SRCS[@]}"

    mkdir -p output/rom
    cp "sdk/src/build/$NAME.bin" "output/rom/$NAME.bin"
    if [ -f "sdk/src/build/$NAME.bin.elf" ]; then
        cp "sdk/src/build/$NAME.bin.elf" "output/rom/$NAME.elf"
    elif [ -f "sdk/src/build/$NAME.elf" ]; then
        cp "sdk/src/build/$NAME.elf" "output/rom/$NAME.elf"
    fi

else
    echo "Error: sdk/src/$NAME.c or sdk/src/$NAME.s does not exist"
    list_programs
    exit 1
fi

echo "Compilation finished. Binary saved to output/rom/$NAME.bin"
exit 0