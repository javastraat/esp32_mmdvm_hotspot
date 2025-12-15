rm -R build
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc,CPUFreq=240,DebugLevel=none,DFUOnBoot=default,EraseFlash=none,EventsCore=1,FlashMode=qio,FlashSize=16M,JTAGAdapter=default,LoopCore=1,MSCOnBoot=default,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,UploadMode=default,UploadSpeed=921600,USBMode=hwcdc,ZigbeeMode=default . --export-binaries --verbose      
cp build/esp32.esp32.esp32s3/esp32_mmdvm_hotspot.ino.bin ./firmware/update.bin
rm -R build
git add firmware/update.bin
git commit -m "Updated update file (update.bin)"
git push

