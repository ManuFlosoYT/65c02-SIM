#!/bin/bash
set -e

# Usage check
if [ ! -d "assets/midi" ]; then
    echo "Error: assets/midi directory not found."
    exit 1
fi

echo "--- Audio to BIN Automation (MIDI & NSF) ---"

target_file=""
MICRO_DOS=false
SDK_MODE=false
MULTITHREAD=false

for arg in "$@"; do
    if [ "$arg" == "--microDOS" ]; then
        MICRO_DOS=true
    fi
    if [ "$arg" == "--sdk" ]; then
        SDK_MODE=true
    fi
    if [ "$arg" == "-multithread" ] || [ "$arg" == "--multithread" ]; then
        MULTITHREAD=true
    fi
done

# Input Handling (filter out --microDOS flag)
if [ "$1" == "all" ] || [ "$2" == "all" ]; then
    shopt -s nullglob
    files=(assets/midi/*.mid assets/midi/*.midi assets/nsf/*.nsf)
    shopt -u nullglob
    
    if [ ${#files[@]} -eq 0 ]; then
        echo "Error: No MIDI files found in assets/midi/"
        exit 1
    fi
elif [ "$#" -eq 0 ]; then
    echo "Available MIDI files:"
    echo "  - all (Compile all MIDI files)"
    shopt -s nullglob
    files=(assets/midi/*.mid assets/midi/*.midi assets/nsf/*.nsf)
    shopt -u nullglob
    
    if [ ${#files[@]} -eq 0 ]; then
        echo "  (No MIDI files found in assets/midi/)"
    else
        for f in "${files[@]}"; do
            echo "  - $(basename "$f")"
        done
    fi
    echo ""
    echo "Usage: $0 <midi_name>"
    exit 0
else
    # Argument provided
    input_arg="$1"
    # Check if exact path or just filename
    if [ -f "$input_arg" ]; then
        target_file="$input_arg"
    elif [ -f "assets/midi/$input_arg" ]; then
        target_file="assets/midi/$input_arg"
    elif [ -f "assets/nsf/$input_arg" ]; then
        target_file="assets/nsf/$input_arg"
    else
        echo "Error: File '$input_arg' not found."
        exit 1
    fi
    files=("$target_file")
fi

# Levels: l1 (2ms) -> l15 (100ms + Chords)
if [ "$SDK_MODE" = true ]; then
    MODES=("l1")
else
    MODES=("l1" "l2" "l3" "l4" "l5" "l6" "l7" "l8" "l9" "l10" "l11" "l12" "l13" "l14" "l15")
fi

pids=()
for midi_file in "${files[@]}"; do
    echo "========================================"
    echo "Processing: $midi_file"
    
    # Determine the expected output name logic
    filename=$(basename "$midi_file")
    filename_no_ext="${filename%.*}"
    clean_name="${filename_no_ext// /}"
    ext="${filename##*.}"
    out_dir="output/midi"
    if [ "${ext,,}" == "nsf" ]; then
        out_dir="output/nsf"
    fi
    
    if [ "$MICRO_DOS" = true ]; then
        echo ">> Generating Raw SID Bytes Mode: L1"
        if [ "$MULTITHREAD" = true ]; then
            python3 tools/sid/audio_to_sid.py "$midi_file" --mode "l1" --microDOS > /dev/null 2>&1 &
            pids+=($!)
        else
            if ! python3 tools/sid/audio_to_sid.py "$midi_file" --mode "l1" --microDOS; then
                echo "   [!] Conversion script failed. Skipping."
                exit -1
            fi
            echo "   [OK] Compilation SUCCESS! (Raw format)"
        fi
        success=true
        continue
    fi

    for mode in "${MODES[@]}"; do
        echo ">> Attempting Mode: $mode"
        
        # 1. Convert MIDI to ASM
        if ! python3 tools/sid/audio_to_sid.py "$midi_file" --mode "$mode"; then
            echo "   [!] Conversion script failed. Skipping."
            break 
        fi
        
        # 2. Compile ASM to BIN
        echo "   Compiling..."
        
        # Ensure build directory exists
        mkdir -p tools/sid/build
        
        if ./scripts/compile-bin.sh "$clean_name" > /dev/null 2>&1; then
            mkdir -p "$out_dir"
            if [ -f "output/rom/$clean_name.bin" ]; then
                mv "output/rom/$clean_name.bin" "$out_dir/$clean_name.bin"
                echo "   [OK] Compilation SUCCESS with mode '$mode'! Moved to $out_dir/$clean_name.bin"
                success=true
                break # Exit mode loop, done with this file
            else
                echo "   [X] Compilation reported success but file not found in output/rom/"
            fi
        else
            echo "   [X] Compilation FAILED (Output too large?)"
        fi
    done
    
    if [ "$success" = false ]; then
        if [ "$SDK_MODE" = true ]; then
            echo "   [!] Song '$clean_name' discarded (Does not fit in Mode L1)."
        else
            echo ">> CRITICAL: Could not compile '$clean_name' even in EXTREME mode."
            exit -1
        fi
    fi
done


if [ "$MULTITHREAD" = true ]; then
    for pid in "${pids[@]}"; do
        wait "$pid" || exit 1
    done
fi

echo "========================================"
echo "All Done!"
exit 0

