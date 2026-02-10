#!/bin/bash
set -e

# Usage check
if [ ! -d "SID" ]; then
    echo "Error: SID directory not found."
    exit 1
fi

echo "--- MIDI to BIN Automation (Smart Optimization) ---"

# Loop through all MIDI files in SID/
shopt -s nullglob
files=(SID/*.mid SID/*.midi)
shopt -u nullglob

if [ ${#files[@]} -eq 0 ]; then
    echo "No MIDI files found in SID/"
    exit 0
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
        if ./compile_bin.sh "$clean_name" > /dev/null 2>&1; then
            echo "   [OK] Compilation SUCCESS with mode '$mode'!"
            success=true
            break # Exit mode loop, done with this file
        else
            echo "   [X] Compilation FAILED (Output too large?)"
        fi
    done
    
    if [ "$success" = false ]; then
        echo ">> CRITICAL: Could not compile '$clean_name' even in EXTREME mode."
    fi
done

echo "========================================"
echo "All Done!"
