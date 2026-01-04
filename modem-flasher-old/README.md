# MMDVM Modem Firmware Flasher

This tool allows you to flash firmware to the MMDVM modem (STM32F103 microcontroller) directly from the ESP32.

## Overview

The MMDVM board contains an STM32F103 microcontroller running the MMDVM_HS firmware. This chip has a built-in bootloader that can be accessed via UART to flash new firmware.

## Hardware Requirements

### Pin Connections

Based on your ESP32-S3 board and MMDVM_HS_Hat:

| Function | ESP32 Pin | MMDVM Pin | Notes |
|----------|-----------|-----------|-------|
| TX (to modem) | GPIO 43 | USART1 RX | Main communication |
| RX (from modem) | GPIO 44 | USART1 TX | Main communication |
| Keep-alive | GPIO 13 | - | Keeps modem awake |
| BOOT0 control | GPIO 4 | BOOT0 | **Critical for flashing** |
| Reset (optional) | - | RESET | Not connected by default |
| Ground | GND | GND | Common ground |

### BOOT0 Pin Connection

**This is the key pin for firmware flashing!**

According to the MMDVM_HS_Hat specifications:
- The BOOT0 pin on the STM32 can be controlled via RPi GPIO20
- On your ESP32-S3 board, GPIO4 maps to this connection
- **You need to verify this connection exists on your board**

When BOOT0 is:
- **LOW (0V)**: STM32 boots into normal user firmware
- **HIGH (3.3V)**: STM32 boots into built-in bootloader mode (ready for flashing)

## How It Works

### Normal Mode (BOOT0 = LOW)
```
ESP32 GPIO4 (LOW) → STM32 BOOT0
                  ↓
            STM32 runs MMDVM_HS firmware
                  ↓
            Responds to MMDVM protocol commands
```

### Bootloader Mode (BOOT0 = HIGH)
```
ESP32 GPIO4 (HIGH) → STM32 BOOT0
                   ↓
             STM32 enters bootloader
                   ↓
             Accepts STM32 bootloader commands
                   ↓
             Can flash new firmware
```

## STM32 Bootloader Protocol

The STM32F103 uses a well-documented UART bootloader protocol (AN3155):

- **Baud rate**: 115200 (configurable, auto-detected by STM32)
- **Parity**: EVEN (8E1 format)
- **Synchronization**: Send 0x7F, receive ACK (0x79)
- **Commands**: Various commands for reading/writing memory

### Supported Commands

| Command | Code | Description |
|---------|------|-------------|
| Get | 0x00 | Get bootloader version and supported commands |
| Get Version | 0x01 | Get bootloader version |
| Get ID | 0x02 | Get chip ID |
| Read Memory | 0x11 | Read from any memory address |
| Go | 0x21 | Execute code at address |
| Write Memory | 0x31 | Write to memory |
| Erase | 0x43 | Erase flash memory |

## Usage

### 1. Upload the Sketch

```bash
# Navigate to the modem-flasher directory
cd modem-flasher

# Open with Arduino IDE or upload via CLI
arduino-cli upload -p /dev/ttyACM0 modem-flasher.ino
```

### 2. Open Serial Monitor

```bash
# Using Arduino IDE: Tools → Serial Monitor (115200 baud)
# Or using screen:
screen /dev/ttyACM0 115200
```

### 3. Run Tests

The sketch provides an interactive menu:

```
================================================
MENU:
================================================
1 - Read current modem firmware version
2 - Enter STM32 bootloader mode
3 - Exit bootloader mode (reboot to normal mode)
4 - Test bootloader connection
5 - Get STM32 chip ID
6 - Get bootloader version
h - Show this menu
================================================
```

### Step-by-Step Workflow

#### A. Verify Current Firmware
1. Press `1` to read the current modem firmware version
2. You should see something like: "MMDVM_HS_Hat-v1.5.2"

#### B. Enter Bootloader Mode
1. Press `2` to enter bootloader mode
2. The sketch will:
   - Set GPIO4 (BOOT0) HIGH
   - Reset the STM32 (if reset pin connected, otherwise you need to power cycle)
   - Configure UART for bootloader communication (8E1)

#### C. Test Bootloader Connection
1. Press `4` to test bootloader connection
2. Expected output:
   ```
   [STEP 1] Sending synchronization byte (0x7F)...
   [STEP 2] Waiting for ACK (0x79)...
   [DEBUG] Received byte: 0x79
   [SUCCESS] Bootloader responded with ACK!
   ```

#### D. Query Chip Information
1. Press `5` to get chip ID
   - Should return: `0x0410` or similar (STM32F103 Medium-density)
2. Press `6` to get bootloader version
   - Should return bootloader version (e.g., `0x22`)

#### E. Flash Firmware (Future Enhancement)
*Not yet implemented - requires writing memory chunks*

The actual flashing process would involve:
1. Erasing flash memory
2. Writing firmware binary in 256-byte chunks
3. Verifying each chunk
4. Resetting to run new firmware

## Troubleshooting

### No ACK from Bootloader

**Problem**: Option 4 shows "No ACK received from bootloader"

**Possible causes**:

1. **GPIO4 not connected to BOOT0**
   - Check your hardware schematic
   - You may need to solder a wire from ESP32 GPIO4 to STM32 BOOT0 pin
   - On RPi-based MMDVM hats, this is typically GPIO20

2. **No RESET pin connected**
   - The STM32 needs to be reset AFTER BOOT0 goes HIGH
   - If no reset pin: manually power cycle the MMDVM board
   - Or press the reset button on the MMDVM board (if available)

3. **Wrong serial configuration**
   - Bootloader requires 8E1 (8 data bits, EVEN parity, 1 stop bit)
   - Make sure you entered bootloader mode first (option 2)

4. **Timing issues**
   - Try adding longer delays after setting BOOT0 HIGH
   - Try power cycling instead of using RESET

### Can't Read Firmware Version

**Problem**: Option 1 shows "No response from modem"

**Possible causes**:

1. **Modem still in bootloader mode**
   - Use option 3 to exit bootloader mode
   - Power cycle the board

2. **Wrong baud rate**
   - MMDVM firmware typically uses 115200
   - Try other common rates: 9600, 38400, 57600

3. **Wiring issues**
   - Verify GPIO43 → MMDVM RX
   - Verify GPIO44 → MMDVM TX
   - Check ground connection

## Next Steps

### Implementing Full Flash Functionality

To actually flash firmware, you would need to:

1. **Prepare firmware binary**
   - Compile MMDVM_HS firmware for your board type
   - Convert to binary format or Intel HEX
   - Store on ESP32 filesystem (SPIFFS/LittleFS) or SD card

2. **Implement Write Memory**
   ```cpp
   void writeMemory(uint32_t address, uint8_t* data, uint16_t length) {
     // Send Write Memory command
     // Send address + checksum
     // Send data + checksum
     // Wait for ACK
   }
   ```

3. **Implement Erase Flash**
   ```cpp
   void eraseFlash() {
     // Send Erase command
     // Send page numbers or global erase
     // Wait for completion (can take several seconds)
   }
   ```

4. **Flash process**
   ```cpp
   void flashFirmware(const uint8_t* firmware, uint32_t size) {
     enterBootloaderMode();
     eraseFlash();

     uint32_t address = 0x08000000; // Flash start
     for (uint32_t i = 0; i < size; i += 256) {
       writeMemory(address + i, firmware + i, 256);
       verifyMemory(address + i, firmware + i, 256);
     }

     exitBootloaderMode();
   }
   ```

### Alternative: Use stm32flash Utility

If GPIO4 is connected to BOOT0, you could also:

1. Connect MMDVM UART to ESP32 USB-Serial bridge
2. Run `stm32flash` from a computer via ESP32
3. This is how Pi-Star does it: `sudo pistar-mmdvmhshatflash hs_hat`

## References

- **AN3155**: USART protocol used in the STM32 bootloader
  - https://www.st.com/resource/en/application_note/an3155-usart-protocol-used-in-the-stm32-bootloader-stmicroelectronics.pdf

- **MMDVM_HS BUILD.md**: Build and flash instructions
  - See: `MMDVM_HS/BUILD.md` in this repository

- **stm32flash utility**: Open source STM32 flasher
  - https://sourceforge.net/projects/stm32flash/

- **STM32F103 Datasheet**: Pin mapping and bootloader details
  - https://www.st.com/resource/en/datasheet/stm32f103c8.pdf

## Hardware Verification

Before using this tool, verify:

1. **Check your board schematic**
   - Is GPIO4 connected to STM32 BOOT0?
   - Is there a RESET pin accessible?

2. **Test with multimeter**
   - Measure GPIO4 voltage when running option 2
   - Should be ~3.3V when BOOT0 HIGH, ~0V when LOW

3. **Check existing flashing method**
   - If you can flash via Pi-Star or stm32flash, GPIO mapping is correct
   - Look at `install_fw_*.sh` scripts in MMDVM_HS/scripts/

## License

Same as main ESP32 MMDVM Hotspot project.

## Contributing

If you successfully flash your modem using this tool:
- Please report your board model and GPIO connections
- Share any modifications needed
- Help improve this documentation
