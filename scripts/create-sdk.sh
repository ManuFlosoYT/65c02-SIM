#!/bin/bash
set -e

SCRIPT_DIR="$(dirname "$0")"

chmod +x "$SCRIPT_DIR/compile-bin.sh" "$SCRIPT_DIR/image-to-bin.sh" "$SCRIPT_DIR/midi-to-bin.sh" "$SCRIPT_DIR/create-cartridge.sh" "$SCRIPT_DIR/create-microdos-cartridge.sh"
"$SCRIPT_DIR/compile-bin.sh" all
"$SCRIPT_DIR/image-to-bin.sh" all
"$SCRIPT_DIR/midi-to-bin.sh" all

# Function to handle ROM flags
get_rom_flags() {
  local name=$1
  local flags=""
  case $name in
    cube|fillVRAM|fullTest|game|room|testGPU|tetris|vga) flags="$flags --gpu true" ;;
  esac
  case $name in
    room) flags="$flags --ips 100000000" ;;
  esac
  case $name in
    fullTest|tetris|testSID|eater) flags="$flags --sid true" ;;
  esac
  case $name in
    testNET) flags="$flags --esp true" ;;
  esac
  echo "$flags"
}

# Package ROMs
echo "Packaging ROMs into cartridges..."
for f in output/rom/*.bin; do
  if [ -f "$f" ]; then
    name=$(basename "${f%.bin}")
    if [ "$name" == "microDOS" ]; then
        continue
    fi
    "$SCRIPT_DIR/create-cartridge.sh" "$f" --name "$name" --author "SDK" --desc "Program for 65c02-SIM" --type rom $(get_rom_flags "$name")
  fi
done

echo "Generating specialized microDOS cartridge..."
"$SCRIPT_DIR/create-microdos-cartridge.sh"

# Package MIDIs
echo "Packaging MIDIs into cartridges..."
for f in output/midi/*.bin; do
  if [ -f "$f" ]; then
    name=$(basename "${f%.bin}")
    "$SCRIPT_DIR/create-cartridge.sh" "$f" --name "$name" --author "SDK" --desc "MIDI file for 65c02-SIM" --ips 1000000 --sid true --type midi
    mkdir -p output/midi
    mv "output/cartridge/$name.65c" "output/midi/"
  fi
done

# Package VRAM images
echo "Packaging VRAM images into cartridges..."
for f in output/vram/*.bin; do
  if [ -f "$f" ]; then
    name=$(basename "${f%.bin}")
    "$SCRIPT_DIR/create-cartridge.sh" --vram "$f" --name "$name" --author "SDK" --desc "VRAM image for 65c02-SIM" --gpu true --type vram
    mkdir -p output/vram
    mv "output/cartridge/$name.65c" "output/vram/"
  fi
done

# Check if zip is installed
if ! command -v zip &> /dev/null; then
    echo "Error: 'zip' command not found. Please install it."
    exit 1
fi

# Create SDK.zip
echo "Creating SDK.zip..."
rm -f SDK.zip 2> /dev/null
zip -r SDK.zip sdk/ tools/ assets/ output/ \
    scripts/compile-bin.sh scripts/image-to-bin.sh scripts/midi-to-bin.sh scripts/create-cartridge.sh scripts/create-microdos-cartridge.sh \
    -x "sdk/src/build/*" \
    -x "create-sdk.sh" \
    -x "tools/sid/__pycache__/*" \
    -x "tools/sid/build/*" \
    -x "sdk/linker/*.o" \
    -x "sdk/msbasic/tmp/*" \
    -x "output/SIM_65C02*" \
    -x "output/web/*" \
    -x "output/video/*" \
    -x "output/savestate/*" \
    -x "output/img/*" \
    -x "output/flac/*"

echo "SDK Build Complete!"
