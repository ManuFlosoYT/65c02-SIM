#!/bin/bash

# Script to create a .65c Cartridge for 65c02-SIM

# Default values
NAME="My Cartridge"
AUTHOR="Unknown"
DESCRIPTION=""
VERSION="1.0"
ROM_FILE=""
VRAM_FILE=""
IPS=1000000
GPU="false"
CYCLE="true"
SID="true"
ESP="false"
SD_ENABLED="false"

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --name) NAME="$2"; shift 2 ;;
        --author) AUTHOR="$2"; shift 2 ;;
        --desc) DESCRIPTION="$2"; shift 2 ;;
        --version) VERSION="$2"; shift 2 ;;
        --rom) ROM_FILE="$2"; shift 2 ;;
        --vram) VRAM_FILE="$2"; shift 2 ;;
        --ips) IPS="$2"; shift 2 ;;
        --gpu) GPU="$2"; shift 2 ;;
        --cycle) CYCLE="$2"; shift 2 ;;
        --sid) SID="$2"; shift 2 ;;
        --esp) ESP="$2"; shift 2 ;;
        --sd) SD_ENABLED="$2"; shift 2 ;;
        *) 
            if [ -z "$ROM_FILE" ] && [[ ! "$1" == --* ]]; then 
                ROM_FILE="$1"
            elif [ -z "$VRAM_FILE" ] && [[ ! "$1" == --* ]]; then
                VRAM_FILE="$1"
            fi
            shift 
            ;;
    esac
done

if [ -z "$ROM_FILE" ] && [ -z "$VRAM_FILE" ]; then
    echo "Usage: $0 <ROM_FILE> [options]"
    echo "Options:"
    echo "  --name \"Game Name\""
    echo "  --author \"Author\""
    echo "  --desc \"Description\""
    echo "  --ips 2000000"
    echo "  --gpu true/false"
    echo "  --cycle true/false"
    echo "  --sid true/false"
    echo "  --esp true/false"
    echo "  --vram \"VRAM_FILE\""
    exit 1
fi

# Determine main name for the cartridge
BASE_FILE="${ROM_FILE:-$VRAM_FILE}"
CARTRIDGE_NAME=$(basename "$BASE_FILE" | cut -d. -f1).65c
OUTPUT_DIR="$(pwd)/output/cartridge"
mkdir -p "$OUTPUT_DIR"
TEMP_DIR=$(mktemp -d)

# Build manifest.json manually to handle optional fields
{
  echo "{"
  echo "  \"version\": \"2.1\","
  echo "  \"metadata\": {"
  echo "    \"name\": \"$NAME\","
  echo "    \"author\": \"$AUTHOR\","
  echo "    \"description\": \"$DESCRIPTION\","
  echo "    \"version\": \"$VERSION\""
  echo "  },"
  [ -n "$ROM_FILE" ] && echo "  \"rom\": \"rom.bin\","
  [ -n "$VRAM_FILE" ] && echo "  \"vram\": \"vram.bin\","
  echo "  \"config\": {"
  echo "    \"target_ips\": $IPS,"
  echo "    \"gpu_enabled\": $GPU,"
  echo "    \"cycle_accurate\": $CYCLE,"
  echo "    \"sid_enabled\": $SID,"
  echo "    \"esp_enabled\": $ESP,"
  echo "    \"sd_enabled\": $SD_ENABLED"
  echo "  },"
  echo "  \"bus\": ["
  echo "    { \"name\": \"RAM\", \"start\": \"0x0000\", \"end\": \"0x7FFF\" },"
  echo "    { \"name\": \"ROM\", \"start\": \"0x8000\", \"end\": \"0xFFFF\" },"
  echo "    { \"name\": \"ACIA\", \"start\": \"0x5000\", \"end\": \"0x5003\" },"
  echo "    { \"name\": \"VIA\", \"start\": \"0x6000\", \"end\": \"0x600F\" },"
  echo "    { \"name\": \"ESP8266\", \"start\": \"0x5004\", \"end\": \"0x5007\" },"
  echo "    { \"name\": \"SID\", \"start\": \"0x4800\", \"end\": \"0x481F\" },"
  echo "    { \"name\": \"GPU\", \"start\": \"0x2000\", \"end\": \"0x3FFF\" }"
  echo "  ]"
  echo "}"
} > "$TEMP_DIR/manifest.json"

FILES_TO_ZIP="manifest.json"

if [ -n "$ROM_FILE" ] && [ -f "$ROM_FILE" ]; then
    cp "$ROM_FILE" "$TEMP_DIR/rom.bin"
    FILES_TO_ZIP="$FILES_TO_ZIP rom.bin"
fi

if [ -n "$VRAM_FILE" ]; then
    cp "$VRAM_FILE" "$TEMP_DIR/vram.bin"
    FILES_TO_ZIP="$FILES_TO_ZIP vram.bin"
fi

# Package as ZIP but rename to .65c
rm -f "$OUTPUT_DIR/$CARTRIDGE_NAME"
(cd "$TEMP_DIR" && zip -r "$OUTPUT_DIR/$CARTRIDGE_NAME" $FILES_TO_ZIP > /dev/null)

rm -rf "$TEMP_DIR"

echo "Cartridge created: output/cartridge/$CARTRIDGE_NAME"
