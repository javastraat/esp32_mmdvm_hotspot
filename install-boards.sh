#!/bin/bash
#
# Install custom board definitions into arduino-esp32 package
# Run this after editing boards/boards.txt or CSV files, and after arduino-esp32 updates
#

set -e

ARDUINO15="$HOME/Library/Arduino15/packages/esp32/hardware/esp32"
PROJECT_BOARDS="$(dirname "$0")/boards"

# Find the installed arduino-esp32 version (use latest if multiple)
ESP32_VERSION=$(ls "$ARDUINO15" | grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | sort -V | tail -1)
if [ -z "$ESP32_VERSION" ]; then
    echo "ERROR: No arduino-esp32 installation found in $ARDUINO15"
    exit 1
fi

TARGET_DIR="$ARDUINO15/$ESP32_VERSION"
PARTITIONS_DIR="$TARGET_DIR/tools/partitions"

echo "arduino-esp32 version: $ESP32_VERSION"
echo "Target: $TARGET_DIR"
echo ""

# Copy boards.txt as boards.local.txt (merged automatically with main boards.txt)
cp "$PROJECT_BOARDS/boards.txt" "$TARGET_DIR/boards.local.txt"
echo "Copied boards/boards.txt -> boards.local.txt"

# Copy partition CSV files
for csv in "$PROJECT_BOARDS"/*.csv; do
    name=$(basename "$csv")
    cp "$csv" "$PARTITIONS_DIR/$name"
    echo "Copied $name -> tools/partitions/"
done

echo ""
echo "Done. Reload Board Data in Arduino IDE (or restart IDE)."
