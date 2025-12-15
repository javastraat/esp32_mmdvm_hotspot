rm -R build
arduino-cli compile --fqbn esp32:esp32:esp32 . --export-binaries --verbose      
cp build/esp32.esp32.esp32/esp32_mmdvm_hotspot.ino.bin ./firmware/update.bin
rm -R build
git add firmware/update.bin
git commit -m "Updated update file (update.bin)"
git push

