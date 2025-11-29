rm -R build
arduino-cli compile --fqbn esp32:esp32:esp32 . --export-binaries --verbose      
cp build/esp32.esp32.esp32/esp32_mmdvm_hotspot.ino.bin ./update_beta.bin
rm -R build
git add update_beta.bin
git commit -m "Updated update file (update_beta.bin)"
git push

