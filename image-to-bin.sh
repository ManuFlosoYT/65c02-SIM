#!/bin/bash

# --- CONFIGURACIÓN ---
GPU_DIR="GPU"
PYTHON_SCRIPT="$GPU_DIR/generator/image-to-bin.py"

# --- FUNCIONES ---
listar_imagenes() {
    echo "-----------------------------------------------------"
    echo "Images in '$GPU_DIR/':"
    echo "-----------------------------------------------------"
    echo "   - all (Compile all images)"
    count=$(find "$GPU_DIR" -maxdepth 1 -type f \( -iname "*.png" -o -iname "*.jpg" -o -iname "*.jpeg" -o -iname "*.bmp" \) | wc -l)

    if [ "$count" -eq 0 ]; then
        echo "   (No images found)"
    else
        find "$GPU_DIR" -maxdepth 1 -type f \( -iname "*.png" -o -iname "*.jpg" -o -iname "*.jpeg" -o -iname "*.bmp" \) -exec basename {} \; | sed 's/^/   - /'
    fi
    echo "-----------------------------------------------------"
}

# --- ARGUMENTOS ---
INPUT_FILE="$1"
TARGET_FILE=""

if [ "$INPUT_FILE" == "all" ]; then
    echo "--- Converting ALL Images ---"
    find "$GPU_DIR" -maxdepth 1 -type f \( -iname "*.png" -o -iname "*.jpg" -o -iname "*.jpeg" -o -iname "*.bmp" \) | while read -r file; do
        $0 "$(basename "$file")"
    done
    exit 0
fi

if [ -z "$INPUT_FILE" ]; then
    echo "Usage: $0 <image_name>"
    listar_imagenes
    exit 1
fi

if [ -f "$INPUT_FILE" ]; then
    TARGET_FILE="$INPUT_FILE"
elif [ -f "$GPU_DIR/$INPUT_FILE" ]; then
    TARGET_FILE="$GPU_DIR/$INPUT_FILE"
else
    echo "Error: File '$INPUT_FILE' not found."
    listar_imagenes
    exit 1
fi

# --- DEPENDENCIAS ---
if ! command -v python3 &> /dev/null; then echo "Python3 not found."; exit 1; fi
if ! python3 -c "import PIL" &> /dev/null; then
    echo "Pillow not found. Please install it using: pip install pillow"
    exit 1
fi

# --- EJECUTAR ---


if python3 "$PYTHON_SCRIPT" "$TARGET_FILE"; then
    echo "Python script finished successfully."
else
    echo "Error: Python script failed."
    exit 1
fi

# Move to output/vram/
FILENAME=$(basename "$TARGET_FILE")
NAME="${FILENAME%.*}"
mkdir -p output/vram
if [ -f "output/$NAME.bin" ]; then
    mv "output/$NAME.bin" "output/vram/$NAME.bin"
    echo "Binary moved to output/vram/$NAME.bin"
else
    echo "Error: Output binary not found."
    exit 1
fi

exit 0