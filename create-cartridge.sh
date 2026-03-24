#!/bin/bash

# Script to create a .65c Cartridge for 65c02-SIM

# Default values
NAME="My Cartridge"
AUTHOR="Unknown"
DESCRIPTION=""
VERSION="1.0"
ROM_FILE=""
IPS=1000000
GPU="false"
CYCLE="true"
SID="true"

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --name) NAME="$2"; shift ;;
        --author) AUTHOR="$2"; shift ;;
        --desc) DESCRIPTION="$2"; shift ;;
        --version) VERSION="$2"; shift ;;
        --rom) ROM_FILE="$2"; shift ;;
        --ips) IPS="$2"; shift ;;
        --gpu) GPU="$2"; shift ;;
        --cycle) CYCLE="$2"; shift ;;
        --sid) SID="$2"; shift ;;
        *) ROM_FILE="$1" ;;
    esac
    shift
done

if [ -z "$ROM_FILE" ]; then
    echo "Usage: $0 <ROM_FILE> [options]"
    echo "Options:"
    echo "  --name \"Game Name\""
    echo "  --author \"Author\""
    echo "  --desc \"Description\""
    echo "  --ips 2000000"
    echo "  --gpu true/false"
    echo "  --cycle true/false"
    echo "  --sid true/false"
    
    echo ""
    read -p "Enter ROM file path: " ROM_FILE
    if [ ! -f "$ROM_FILE" ]; then echo "File not found!"; exit 1; fi
    
    read -p "Enter Game Name [$NAME]: " input_name
    NAME=${input_name:-$NAME}
    
    read -p "Enter Author [$AUTHOR]: " input_author
    AUTHOR=${input_author:-$AUTHOR}
    
    read -p "Enter Description [$DESCRIPTION]: " input_desc
    DESCRIPTION=${input_desc:-$DESCRIPTION}
    
    read -p "Enter Target IPS [$IPS]: " input_ips
    IPS=${input_ips:-$IPS}
    
    read -p "Enter GPU Enabled (true/false) [$GPU]: " input_gpu
    GPU=${input_gpu:-$GPU}
    
    read -p "Enter Cycle Accurate (true/false) [$CYCLE]: " input_cycle
    CYCLE=${input_cycle:-$CYCLE}

    read -p "Enter SID Enabled (true/false) [$SID]: " input_sid
    SID=${input_sid:-$SID}
fi

if [ ! -f "$ROM_FILE" ]; then
    echo "Error: ROM file '$ROM_FILE' not found!"
    exit 1
fi

CARTRIDGE_NAME=$(basename "$ROM_FILE" | cut -d. -f1).65c
OUTPUT_DIR="$(pwd)/output/cartridge"
mkdir -p "$OUTPUT_DIR"
TEMP_DIR=$(mktemp -d)

# Create manifest.json
cat <<EOF > "$TEMP_DIR/manifest.json"
{
  "version": "1.0",
  "metadata": {
    "name": "$NAME",
    "author": "$AUTHOR",
    "description": "$DESCRIPTION",
    "version": "$VERSION"
  },
  "rom": "rom.bin",
  "config": {
    "target_ips": $IPS,
    "gpu_enabled": $GPU,
    "cycle_accurate": $CYCLE,
    "sid_enabled": $SID
  }
}
EOF

cp "$ROM_FILE" "$TEMP_DIR/rom.bin"

# Package as ZIP but rename to .65c
(cd "$TEMP_DIR" && zip -r "$OUTPUT_DIR/$CARTRIDGE_NAME" manifest.json rom.bin > /dev/null)

rm -rf "$TEMP_DIR"

echo "Cartridge created: output/cartridge/$CARTRIDGE_NAME"
