#!/bin/bash
set -e

# Automation script for microDOS Cartridge version 3.0
# Requires: dosfstools (mkfs.fat), mtools (mformat, mcopy)

echo "--- Building microDOS Cartridge v3.0 ---"

# 1. Compile microDOS core
./compile-bin.sh microDOS

# 2. Compile all available microDOS apps
echo "  Compiling microDOS apps..."
mkdir -p output/apps
for app_src in Binaries/Apps/*.c; do
    if [ -f "$app_src" ]; then
        app_name=$(basename "${app_src%.c}")
        echo "    Building $app_name.app..."
        ./compile-bin.sh "$app_name" --microDOS > /dev/null 2>&1
    fi
done

# 3. Create FAT16 SD Image (32MB)
echo "  Creating 32MB SD Card Image..."
mkdir -p output/img
SD_PATH="output/img/microDOS.img"
rm -f "$SD_PATH"
truncate -s 32M "$SD_PATH"
# Use mkfs.fat with parameters matching the emulator's CreateFAT16Image logic:
# -F 16: FAT16
# -R 4: 4 reserved sectors
# -s 8: 8 sectors per cluster
# -r 512: 512 root entries
# -S 512: 512 bytes per sector
# -n: Volume label
mkfs.fat -F 16 -R 4 -s 8 -r 512 -S 512 -n "MICRODOS" "$SD_PATH" > /dev/null

# 4. Compile MIDs to .sid for microDOS
echo "  Compiling raw SID files..."
./midi-to-bin.sh all --microDOS

# 5. Populate SD Card with /bin, /sid, and apps
echo "  Populating SD Card..."
# Use mtools (mmd, mcopy) to manipulate the raw image
mmd -i "$SD_PATH" ::/bin
mmd -i "$SD_PATH" ::/sid

for app in output/apps/*.app; do
    if [ -f "$app" ]; then
        mcopy -i "$SD_PATH" "$app" ::/bin/
    fi
done

for sidfile in output/midi/*.sid; do
    if [ -f "$sidfile" ]; then
        mcopy -i "$SD_PATH" "$sidfile" ::/sid/
    fi
done

# 5. Create the Cartridge (.65c)
echo "  Packaging Cartridge..."
./create-cartridge.sh output/rom/microDOS.bin \
    --sd-image "$SD_PATH" \
    --name "microDOS" \
    --author "ManuFloso" \
    --desc "microDOS Operating System with pre-loaded apps" \
    --version "3.0" \
    --ips 1000000 \
    --sid true \
    --sd true \
    --esp true

echo "Done! Cartridge created at output/cartridge/microDOS.65c"
