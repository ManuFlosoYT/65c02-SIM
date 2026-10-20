#!/bin/bash

# Script to create a .65c Cartridge for 65c02-SIM

# Default values
NAME="My Cartridge"
AUTHOR="Unknown"
DESCRIPTION=""
VERSION="1.1"
TYPE="rom"
ROM_FILE=""
DBG_FILE=""
ELF_FILE=""
VRAM_FILE=""
SD_IMAGE=""
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
        --dbg) DBG_FILE="$2"; shift 2 ;;
        --elf) ELF_FILE="$2"; shift 2 ;;
        --vram) VRAM_FILE="$2"; shift 2 ;;
        --sd-image) SD_IMAGE="$2"; shift 2 ;;
        --ips) IPS="$2"; shift 2 ;;
        --gpu) GPU="$2"; shift 2 ;;
        --cycle) CYCLE="$2"; shift 2 ;;
        --sid) SID="$2"; shift 2 ;;
        --esp) ESP="$2"; shift 2 ;;
        --sd) SD_ENABLED="$2"; shift 2 ;;
        --type) TYPE="$2"; shift 2 ;;
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
    echo "  --dbg \"rom.dbg\""
    echo "  --elf \"rom.elf\""
    echo "  --ips 2000000"
    echo "  --gpu true/false"
    echo "  --cycle true/false"
    echo "  --sid true/false"
    echo "  --esp true/false"
    echo "  --vram \"VRAM_FILE\""
    echo "  --sd-image \"sdcard.img\""
    exit 1
fi

# Determine main name for the cartridge
BASE_FILE="${ROM_FILE:-$VRAM_FILE}"
BASENAME=$(basename "$BASE_FILE")
CARTRIDGE_NAME="${BASENAME%.*}.65c"
OUTPUT_DIR="$(pwd)/output/cartridge"
mkdir -p "$OUTPUT_DIR"
TEMP_DIR=$(mktemp -d)

if [ -z "$DBG_FILE" ] && [ -z "$ELF_FILE" ] && [ -n "$ROM_FILE" ]; then
    base_no_ext="${ROM_FILE%.*}"
    base_name=$(basename "$base_no_ext")
    for cand in \
        "${base_no_ext}.dbg" \
        "${base_no_ext}.elf" \
        "${base_no_ext}.bin.elf" \
        "sdk/src/build/${base_name}.bin.elf" \
        "sdk/src/build/${base_name}.elf" \
        "sdk/src/build/${base_name}.dbg" \
        "tools/sid/build/${base_name}.bin.elf" \
        "tools/sid/build/${base_name}.elf" \
        "tools/sid/build/${base_name}.dbg"; do
        if [ -f "$cand" ]; then
            if [[ "$cand" == *.dbg ]]; then
                DBG_FILE="$cand"
            else
                ELF_FILE="$cand"
            fi
            break
        fi
    done
fi

if [ -n "$DBG_FILE" ] && [ ! -f "$DBG_FILE" ]; then
    DBG_FILE=""
fi
if [ -n "$ELF_FILE" ] && [ ! -f "$ELF_FILE" ]; then
    ELF_FILE=""
fi

# Build manifest.json manually to handle optional fields
{
  echo "{"
  echo "  \"version\": \"2.2\","
  echo "  \"metadata\": {"
  echo "    \"name\": \"$NAME\","
  echo "    \"author\": \"$AUTHOR\","
  echo "    \"description\": \"$DESCRIPTION\","
  echo "    \"version\": \"$VERSION\","
  echo "    \"type\": \"$TYPE\""
  echo "  },"
  [ -n "$ROM_FILE" ] && echo "  \"rom\": \"rom.bin\","
  [ -n "$DBG_FILE" ] && echo "  \"dbg\": \"rom.dbg\","
  [ -n "$ELF_FILE" ] && echo "  \"elf\": \"rom.elf\","
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
  echo "    { \"name\": \"SD Card\", \"start\": \"0x5008\", \"end\": \"0x500B\" },"
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

if [ -n "$DBG_FILE" ] && [ -f "$DBG_FILE" ]; then
    cp "$DBG_FILE" "$TEMP_DIR/rom.dbg"
    FILES_TO_ZIP="$FILES_TO_ZIP rom.dbg"
fi

if [ -n "$ELF_FILE" ] && [ -f "$ELF_FILE" ]; then
    cp "$ELF_FILE" "$TEMP_DIR/rom.elf"
    FILES_TO_ZIP="$FILES_TO_ZIP rom.elf"
fi

if [ -n "$VRAM_FILE" ]; then
    cp "$VRAM_FILE" "$TEMP_DIR/vram.bin"
    FILES_TO_ZIP="$FILES_TO_ZIP vram.bin"
fi

if [ -n "$SD_IMAGE" ] && [ -f "$SD_IMAGE" ]; then
    cp "$SD_IMAGE" "$TEMP_DIR/sdcard.img"
    FILES_TO_ZIP="$FILES_TO_ZIP sdcard.img"
fi

# Package as ZIP but rename to .65c
rm -f "$OUTPUT_DIR/$CARTRIDGE_NAME"
(cd "$TEMP_DIR" && zip -r "$OUTPUT_DIR/$CARTRIDGE_NAME" $FILES_TO_ZIP > /dev/null)

rm -rf "$TEMP_DIR"

echo "Cartridge created: output/cartridge/$CARTRIDGE_NAME"
