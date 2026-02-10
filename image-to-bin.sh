#!/bin/bash

# --- CONFIGURACIÓN ---
GPU_DIR="GPU"
PYTHON_SCRIPT="$GPU_DIR/image-to-bin.py"

# --- FUNCIONES ---
listar_imagenes() {
    echo "-----------------------------------------------------"
    echo "Images in '$GPU_DIR/':"
    echo "-----------------------------------------------------"
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

if [ -z "$INPUT_FILE" ]; then
    echo "Error: Missing file."
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
    echo "Installing Pillow..."
    pip install pillow
fi

# --- EJECUTAR ---
python3 "$PYTHON_SCRIPT" "$TARGET_FILE"