#!/bin/bash
set -e

if [ ! -d "assets/midi" ]; then
    echo "Error: assets/midi directory not found."
    exit 1
fi

echo "--- Audio to BIN Automation (MIDI & NSF) ---"

target_file=""
MICRO_DOS=false
SDK_MODE=false
MULTITHREAD=false
EXTRA_ARGS=""
input_target=""

prev_arg=""
for arg in "$@"; do
    if [ -n "$prev_arg" ]; then
        EXTRA_ARGS="$EXTRA_ARGS $prev_arg $arg"
        prev_arg=""
        continue
    fi

    if [ "$arg" == "--microDOS" ]; then
        MICRO_DOS=true
    elif [ "$arg" == "--sdk" ]; then
        SDK_MODE=true
    elif [ "$arg" == "-multithread" ] || [ "$arg" == "--multithread" ]; then
        MULTITHREAD=true
    elif [ "$arg" == "--chip" ] || [ "$arg" == "--system" ]; then
        prev_arg="$arg"
    elif [[ "$arg" != -* ]]; then
        input_target="$arg"
    fi
done

# Input Handling
if [ "$input_target" == "all" ]; then
    shopt -s nullglob
    files=(assets/midi/*.mid assets/midi/*.midi assets/nsf/*.nsf)
    shopt -u nullglob

    if [ ${#files[@]} -eq 0 ]; then
        echo "Error: No MIDI files found in assets/midi/"
        exit 1
    fi
elif [ -z "$input_target" ]; then
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
    echo "Usage: $0 <midi_name> [options]"
    exit 0
else
    # Check if exact path or just filename
    if [ -f "$input_target" ]; then
        target_file="$input_target"
    elif [ -f "assets/midi/$input_target" ]; then
        target_file="assets/midi/$input_target"
    elif [ -f "assets/nsf/$input_target" ]; then
        target_file="assets/nsf/$input_target"
    else
        echo "Error: File '$input_target' not found."
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

    success=false
    if [ "$MICRO_DOS" = true ]; then
        echo ">> Generating Raw SID Bytes Mode: L1"
        if [ "$MULTITHREAD" = true ]; then
            python3 tools/sid/audio_to_sid.py "$midi_file" --mode "l1" --microDOS $EXTRA_ARGS > /dev/null 2>&1 &
            pids+=($!)
        else
            if ! python3 tools/sid/audio_to_sid.py "$midi_file" --mode "l1" --microDOS $EXTRA_ARGS; then
                echo "   [!] Conversion script failed. Skipping."
                exit -1
            fi
            echo "   [OK] Compilation SUCCESS! (Raw format)"
        fi
        success=true
        continue
    fi

    if [ "${ext,,}" == "nsf" ] || [ "$SDK_MODE" = true ]; then
        current_modes=("l1")
    else
        current_modes=("${MODES[@]}")
    fi

    for mode in "${current_modes[@]}"; do
        echo ">> Attempting Mode: $mode"

        # Convert MIDI to ASM
        if ! python3 tools/sid/audio_to_sid.py "$midi_file" --mode "$mode" $EXTRA_ARGS; then
            echo "   [!] Conversion script failed. Skipping."
            break
        fi

        # Compile ASM to BIN
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
        if [ "${ext,,}" == "nsf" ]; then
            echo "   [!] NSF song '$clean_name' discarded for independent ROM (Exceeds L1 limit)."
        elif [ "$SDK_MODE" = true ]; then
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
