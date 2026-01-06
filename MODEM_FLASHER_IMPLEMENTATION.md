# MMDVM Modem Firmware Flasher Implementation

## Overview

This document describes the integration of MMDVM modem firmware flashing capability into the ESP32 MMDVM Hotspot admin interface.

## Features

- **Option 1**: Upload firmware from local file (.bin)
- **Option 2**: Flash firmware directly from GitHub URLs (predefined or custom)
- Real-time progress tracking during flash operation
- Status messages and error reporting
- Automatic modem version detection after flash

## Files Modified

### 1. `config.h`
Added STM32 bootloader pin definitions:
```cpp
#define MMDVM_BOOT0_PIN 4    // BOOT0 pin (HIGH = bootloader, LOW = normal)
#define MMDVM_RESET_PIN 13   // RESET pin (LOW = reset, HIGH = run)
```

### 2. `esp32_mmdvm_hotspot.ino`
- Added include: `#include "include/modem_flasher.h"`
- Endpoints already registered in `setupWebServer()`:
  - `/flash-modem-upload` (POST with file upload)
  - `/flash-modem-url` (POST with URL parameter)

### 3. `web/pages/admin.h`
Added "MMDVM Modem Firmware" card with:
- Current modem version display
- File upload form
- URL dropdown with predefined firmware versions:
  - Single MMDVM v1.6.1
  - Dual MMDVM v1.6.1  
  - Single MMDVM v1.5.2
  - Custom URL option
- Progress bar with percentage display
- Status message display
- JavaScript handlers for both upload methods

### 4. `include/modem_flasher.h` (NEW)
Complete STM32 bootloader protocol implementation:

#### Main Handler Functions:
- `handleFlashModemUpload()` - Processes multipart file uploads
- `handleFlashModemURL()` - Downloads from URL and flashes

#### Helper Functions:
- `modemEnterBootloader()` - Hardware reset sequence into bootloader mode
- `modemExitBootloader()` - Hardware reset back to normal mode
- `modemSyncBootloader()` - Send 0x7F sync byte, wait for ACK
- `modemEraseFlash()` - Extended erase command (global erase 0xFFFF)
- `modemWriteMemory()` - Write 256-byte chunks with address and data checksums
- `modemSendCommand()` - Send command byte + complement, wait for ACK
- `modemWaitForAck()` - Wait for 0x79 (ACK) or 0x1F (NACK) response

## STM32 Bootloader Protocol

### Constants
- **SYNC_BYTE**: 0x7F
- **ACK**: 0x79
- **NACK**: 0x1F

### Commands
- **GET** (0x00): Get bootloader version and supported commands
- **GID** (0x02): Get chip ID  
- **RM** (0x11): Read memory
- **WM** (0x31): Write memory
- **EE** (0x44): Extended erase

### Flash Memory
- **Start Address**: 0x08000000
- **Write Size**: 256 bytes per chunk
- **Padding**: 0xFF for partial chunks

### Hardware Control Sequence

#### Enter Bootloader Mode:
1. Stop UART communication
2. Set BOOT0 = HIGH (GPIO 4)
3. Assert RESET = LOW (GPIO 13)
4. Release RESET = HIGH
5. Wait 500ms
6. Restart UART in 8E1 mode (even parity for bootloader)

#### Exit Bootloader Mode:
1. Stop UART communication
2. Set BOOT0 = LOW (GPIO 4)
3. Assert RESET = LOW (GPIO 13)
4. Release RESET = HIGH
5. Wait 500ms
6. Restart UART in 8N1 mode (no parity for normal operation)

## Flash Process Flow

### File Upload Method:
1. User selects .bin file and clicks "Upload and Flash"
2. JavaScript sends POST to `/flash-modem-upload` with multipart form data
3. Server enters bootloader mode
4. Server syncs with bootloader (sends 0x7F)
5. Server erases flash (10-30 seconds)
6. Server streams file in 256-byte chunks directly to STM32
7. Server exits bootloader mode
8. Response sent to client with success/error message

### URL Download Method:
1. User selects firmware from dropdown (or enters custom URL) and clicks "Flash from URL"
2. JavaScript sends POST to `/flash-modem-url` with URL parameter
3. Server downloads firmware using HTTPClient
4. Server enters bootloader mode
5. Server syncs with bootloader
6. Server erases flash
7. Server streams download directly to STM32 (no temp file)
8. Server exits bootloader mode
9. Response sent to client with success/error message

## Timing Considerations

- **Bootloader sync**: 1-2 second timeout, 5 retry attempts
- **Flash erase**: Up to 60 seconds (typically 10-30 seconds)
- **Write operations**: 2 second timeout per 256-byte chunk
- **Hardware reset delays**: 100-500ms between GPIO state changes

## Error Handling

The implementation includes comprehensive error handling:
- ACK/NACK detection on all bootloader operations
- Timeout detection with appropriate error messages
- HTTP download error handling
- File upload abort handling
- Automatic bootloader exit on errors

## Security Notes

- Firmware flashing is only available in the admin panel
- Admin panel requires authentication (username/password)
- No authentication bypass for firmware flash endpoints
- Operations are logged via `logSerial()` for troubleshooting

## Testing Checklist

- [ ] Test file upload with valid .bin firmware
- [ ] Test URL download from GitHub
- [ ] Verify progress bar updates correctly
- [ ] Test with invalid file (should show error)
- [ ] Test with invalid URL (should show error)  
- [ ] Test modem communication after successful flash
- [ ] Test modem version detection after flash
- [ ] Test abort/timeout scenarios
- [ ] Verify logs show all operations

## Predefined Firmware URLs

Currently configured URLs in admin.h:
```javascript
const firmwareUrls = {
  'single-1.6.1': 'https://github.com/juribeparada/MMDVM_HS/releases/download/v1.6.1/install_fw_stm32f1_hs.bin',
  'dual-1.6.1': 'https://github.com/juribeparada/MMDVM_HS/releases/download/v1.6.1/install_fw_stm32f1_hs_dual_hat.bin',
  'single-1.5.2': 'https://github.com/juribeparada/MMDVM_HS/releases/download/v1.5.2/install_fw_stm32f1_hs.bin'
};
```

## Future Enhancements

Potential improvements:
- Add modem version detection before flash to compare versions
- Implement firmware verification after flash (read-back check)
- Add support for additional modem types (STM32F4, etc.)
- Create backup of existing firmware before flash
- Add batch flash capability for multiple modems
- Implement OTA firmware update for ESP32 itself in same interface

## References

- STM32 Bootloader AN3155: STM32 USART protocol specification
- MMDVM_HS Firmware: https://github.com/juribeparada/MMDVM_HS
- Original modem-flasher-web implementation: `test-sketches/modem-flasher-web/modem-flasher-web.ino`
