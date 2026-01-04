# MMDVM Modem Firmware Flashing - Findings & Next Steps

## Hardware Configuration

**Your Setup:**
- ESP32-S3 board
- MMDVM_HS_Hat modem board
- MMDVM_HS_Hat IO20 → ESP32 GPIO4 (BOOT0 - controls bootloader mode)
- MMDVM_HS_Hat IO21 → ESP32 GPIO13 (RESET - hardware reset pin!)

**BREAKTHROUGH DISCOVERY:**
GPIO13 is NOT just for "keep-alive" - it's the actual STM32 RESET pin!
This was confirmed by examining the Raspberry Pi flash script which uses:
`stm32flash -i 20,-21,21:-20,21` meaning GPIO20=BOOT0, GPIO21=RESET

## What Works ✅

1. **Normal MMDVM Communication**
   - Successfully reading modem firmware version
   - Version detected: `MMDVM_HS_Hat-v1.6.1 20231115_WPSD 14.7456MHz ADF7021`
   - GPIO13 keep-alive working perfectly
   - SVC LED blinks when modem is active

2. **GPIO Control**
   - GPIO4 can be set HIGH/LOW from ESP32
   - GPIO13 controls modem wake/sleep (SVC LED responds)
   - Stopping GPIO13 makes modem sleep (SVC LED stops)

3. **Serial Communication**
   - UART on GPIO43/44 works at 115200 baud
   - Both 8N1 (normal) and 8E1 (bootloader) configurations work
   - Can send and receive MMDVM protocol frames

## What Doesn't Work ❌

1. **STM32 Bootloader Access**
   - No ACK response (0x79) when sending sync byte (0x7F)
   - Tested at multiple baud rates: 115200, 57600, 9600
   - Tested with 8E1 parity (required by STM32 bootloader)
   - No response at all (no bytes received)

2. **Soft Reset via GPIO13**
   - Stopping GPIO13 makes modem sleep
   - Restarting GPIO13 wakes modem
   - But modem boots back into normal firmware, not bootloader
   - Suggests sleep mode, not actual reset

## Root Cause Analysis

### Most Likely: GPIO4 Not Connected to BOOT0

**Evidence:**
1. No bootloader response despite multiple attempts
2. GPIO4 set HIGH before wake attempts
3. All UART communication working correctly
4. STM32 should respond immediately if BOOT0 is HIGH during boot

**Theory:**
- The MMDVM_HS_Hat board may not expose BOOT0 to IO20
- BOOT0 might be hardwired LOW on the board
- Or IO20 serves a different function on this specific board variant

### Alternative: Need Hardware Reset

**Evidence:**
1. Soft reset via GPIO13 doesn't trigger bootloader
2. STM32 only checks BOOT0 during actual reset/power-on
3. No RESET pin identified in current configuration

**Theory:**
- GPIO13 wake/sleep is not a true reset
- May need to find and control STM32 nRST pin
- Or require physical power cycling

## Verification Tests Performed

| Test | Method | Baud Rate | Result |
|------|--------|-----------|--------|
| Read Version | Normal MMDVM | 115200 | ✅ SUCCESS |
| Bootloader Sync | 8E1, GPIO4 HIGH | 115200 | ❌ No ACK |
| Bootloader Sync | 8E1, GPIO4 HIGH | 57600 | ❌ No ACK |
| Bootloader Sync | 8E1, GPIO4 HIGH | 9600 | ❌ No ACK |
| Soft Reset Test | GPIO13 cycle | - | ❌ Boots normal FW |

## Recommended Next Steps

### Option 1: Verify Hardware Connections ⭐ RECOMMENDED

**Check the MMDVM_HS_Hat schematic:**
1. Is BOOT0 actually connected to the 40-pin header?
2. Which pin is BOOT0? (might not be IO20)
3. Is there an nRST (reset) pin exposed?

**Use a multimeter:**
1. Find the STM32 BOOT0 pin on the chip
2. Measure voltage when GPIO4 is HIGH/LOW
3. See if voltage changes (if yes, connection exists)

**Check board documentation:**
- Look for "firmware update" procedures in MMDVM_HS_Hat docs
- See how Pi-Star/WPSD updates the modem firmware
- Check if there's a jumper or switch for bootloader mode

### Option 2: Find RESET Pin

**Possible candidates:**
- RPi GPIO5 (common for reset on Pi hats)
- RPi GPIO6
- RPi GPIO12
- RPi GPIO16

**Test method:**
- Add option to test different GPIO pins as RESET
- Try toggling each pin LOW then HIGH
- See if modem restarts (SVC LED pattern changes)

### Option 3: Use stm32flash from ESP32

**Alternative approach:**
If bootloader access isn't possible, implement stm32flash protocol:
1. The ESP32 could run the stm32flash algorithm
2. Read .bin firmware from SD card or SPIFFS
3. Flash it to the STM32 via UART

**Advantages:**
- Don't need to control BOOT0/RESET
- Can use existing flash tools' protocol
- Well-documented process

### Option 4: External Flashing

**As a fallback:**
1. Use a USB-to-Serial adapter
2. Connect directly to STM32 USART1
3. Use stm32flash from a computer
4. Requires accessing BOOT0 jumper on the board

## Code Improvements Made

### Test Sketch Features ✅

1. **Read modem firmware version** - Working perfectly
2. **Enter bootloader mode** - Sets BOOT0 HIGH, waits for reset
3. **Test bootloader** - Multiple baud rates, shows debug output
4. **Soft reset** - Automated GPIO13 cycling
5. **Monitor raw UART** - Debug tool to see all bytes
6. **Force bootloader test** - Bypass mode checks

### Current Capabilities

- ✅ Full MMDVM protocol communication
- ✅ GPIO control (BOOT0, wake/sleep)
- ✅ Multi-baud rate testing
- ✅ Detailed debug output
- ✅ Error handling and diagnostics
- ❌ Actual bootloader access (hardware limitation)

## Resources & References

### MMDVM_HS Documentation
- Build instructions: `MMDVM_HS/BUILD.md`
- Flashing methods: Uses `stm32flash` utility
- Pi-Star scripts: `/MMDVM_HS/scripts/install_fw_*.sh`

### STM32 Bootloader
- Protocol: AN3155 Application Note
- Sync byte: 0x7F
- ACK: 0x79 (not received)
- Baud rates: Auto-detected by STM32

### Hardware Mapping
- MMDVM RX (PA10) → ESP32 TX (GPIO43) ✅
- MMDVM TX (PA9) → ESP32 RX (GPIO44) ✅
- Keep-alive → ESP32 GPIO13 ✅
- BOOT0 → ESP32 GPIO4 ❓ (unverified)
- RESET → Unknown ❓

## THE SOLUTION! ✅

After examining the Raspberry Pi flash scripts in `MMDVM_HS/scripts/install_fw_hshat.sh`, we discovered:

```bash
sudo stm32flash -v -w firmware.bin -g 0x0 -R -i 20,-21,21:-20,21 /dev/ttyAMA0
```

The `-i` flag sequence means:
- **Entry sequence:** `20,-21,21`
  1. GPIO 20 HIGH (BOOT0 = HIGH - enable bootloader)
  2. GPIO 21 LOW (RESET = assert reset)
  3. GPIO 21 HIGH (RESET = release - STM32 boots into bootloader)

- **Exit sequence:** `-20,21`
  1. GPIO 20 LOW (BOOT0 = LOW - normal mode)
  2. GPIO 21 toggled (reset to boot normal firmware)

**This means:**
- RPi GPIO 20 = **BOOT0** → Maps to ESP32 **GPIO 4** ✅
- RPi GPIO 21 = **RESET** → Maps to ESP32 **GPIO 13** ✅

**GPIO 13 is dual-purpose:**
1. When toggled LOW→HIGH: Hardware RESET
2. When kept active with UART: Keep-alive (prevents sleep)

## Updated Code

The modem-flasher now includes:
- `hardwareResetToBootloader()` - Exact stm32flash sequence for bootloader entry
- `hardwareResetToNormal()` - Reset sequence to return to normal firmware
- Updated `enterBootloaderMode()` - Uses hardware reset (option 2)
- Updated `forceBootloaderTest()` - Uses hardware reset (option 0)

## Conclusion

The test suite successfully demonstrates:
1. ✅ ESP32 can communicate with MMDVM firmware
2. ✅ ESP32 can control modem wake/sleep
3. ✅ UART and GPIO control working perfectly
4. ✅ **Hardware reset sequence identified and implemented!**

**Primary Discovery:** GPIO13 is the RESET pin (not just keep-alive), GPIO4 is BOOT0.

**Next Action:** Test options 2 and 0 to verify bootloader access with hardware reset!

## Files Created

- `modem-flasher.ino` - Full diagnostic and flashing tool
- `README.md` - Documentation and usage guide
- `FINDINGS.md` - This summary document

## Contact & Support

If you successfully access the bootloader:
- Please document the exact GPIO pins used
- Share any hardware modifications needed
- Update this document with findings

---

**Document Version:** 1.0
**Date:** 2026-01-04
**Status:** Investigation complete, hardware verification needed
