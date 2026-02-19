#!/bin/bash
set -e

# Usage check
if [ ! -d "SID" ]; then
    echo "Error: SID directory not found."
    exit 1
fi

echo "--- MIDI to BIN Automation (Smart Optimization) ---"

# Input Handling
target_file=""

if [ "$1" == "all" ]; then
    shopt -s nullglob
    files=(SID/*.mid SID/*.midi)
    shopt -u nullglob
    
    if [ ${#files[@]} -eq 0 ]; then
        echo "Error: No MIDI files found in SID/"
        exit 1
    fi
elif [ "$#" -eq 0 ]; then
    echo "Available MIDI files:"
    echo "  - all (Compile all MIDI files)"
    shopt -s nullglob
    files=(SID/*.mid SID/*.midi)
    shopt -u nullglob
    
    if [ ${#files[@]} -eq 0 ]; then
        echo "  (No MIDI files found in SID/)"
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
    elif [ -f "SID/$input_arg" ]; then
        target_file="SID/$input_arg"
    else
        echo "Error: File '$input_arg' not found."
        exit 1
    fi
    files=("$target_file")
fi

# Levels: l1 (2ms) -> l8 (100ms + Chords)
MODES=("l1" "l2" "l3" "l4" "l5" "l6" "l7" "l8")

for midi_file in "${files[@]}"; do
    echo "========================================"
    echo "Processing: $midi_file"
    
    # Determine the expected output name logic
    filename=$(basename "$midi_file")
    filename_no_ext="${filename%.*}"
    clean_name="${filename_no_ext// /}"
    
    success=false
    
    for mode in "${MODES[@]}"; do
        echo ">> Attempting Mode: $mode"
        
        # 1. Convert MIDI to ASM
        if ! python3 SID/midi_to_sid.py "$midi_file" --mode "$mode"; then
            echo "   [!] Conversion script failed. Skipping."
            break 
        fi
        
        # 2. Compile ASM to BIN
        echo "   Compiling..."
        if ./compile-bin.sh "$clean_name" > /dev/null 2>&1; then
            mkdir -p output/midi
            if [ -f "output/rom/$clean_name.bin" ]; then
                mv "output/rom/$clean_name.bin" "output/midi/$clean_name.bin"
                echo "   [OK] Compilation SUCCESS with mode '$mode'! Moved to output/midi/$clean_name.bin"
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
        echo ">> CRITICAL: Could not compile '$clean_name' even in EXTREME mode."
        exit -1
    fi
done


echo "========================================"
echo "All Done!"
exit 0

