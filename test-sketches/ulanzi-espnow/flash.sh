esptool.py --chip esp32 --port /dev/cu.usbserial-110 --baud 115200 write_flash -z \
  0x1000 ulanzi-espnow.ino.bootloader.bin \
  0x8000 ulanzi-espnow.ino.partitions.bin \
  0xe000 boot_app0.bin \
  0x10000 ulanzi-espnow.ino.bin
