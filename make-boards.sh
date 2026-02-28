rm -f boards/esp32mmdvm-1.0.0.zip
cd esp32mmdvm/hardware/esp32mmdvm/1.0.0
zip -r ../../../../boards/esp32mmdvm-1.0.0.zip . -x "*.DS_Store" -x "__MACOSX/*"
cd ../../../..

SIZE=$(stat -f%z boards/esp32mmdvm-1.0.0.zip)
HASH=$(shasum -a 256 boards/esp32mmdvm-1.0.0.zip | awk '{print $1}')

echo "Size: $SIZE"
echo "SHA-256: $HASH"

JSON="boards/package_esp32mmdvm_index.json"
sed -i '' "s/\"checksum\": \"SHA-256:[^\"]*\"/\"checksum\": \"SHA-256:$HASH\"/" "$JSON"
sed -i '' "s/\"size\": [0-9]*/\"size\": $SIZE/" "$JSON"

echo "Updated $JSON"
