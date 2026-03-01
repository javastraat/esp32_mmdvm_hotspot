/*
 * modem_flasher.cpp - MMDVM Modem Firmware Flasher Implementation
 */

#include "include/modem_flasher.h"
#include "system/system_logger.h"

// Global state variables
bool modemInBootloaderMode = false;
bool modemFlashInProgress = false;
int modemFlashProgress = 0;
String modemFlashStatus = "";
uint8_t* modemFirmwareBuffer = nullptr;
size_t modemFirmwareSize = 0;

// Firmware update progress tracking (for OLED display)
volatile bool modemFirmwareUpdateActive = false;
volatile int modemFirmwareUpdateProgress = 0;
volatile unsigned long modemFirmwareBytesTotal = 0;
volatile unsigned long modemFirmwareBytesWritten = 0;

volatile bool espFirmwareUpdateActive = false;
volatile int espFirmwareUpdateProgress = 0;
volatile unsigned long espFirmwareBytesTotal = 0;
volatile unsigned long espFirmwareBytesWritten = 0;
volatile bool espFirmwareReadyToFlash = false;

// Private upload state
static uint32_t modemUploadAddress = FLASH_START_ADDR;
static size_t modemUploadBytesWritten = 0;
static uint8_t modemUploadBuffer[MAX_WRITE_SIZE];
static size_t modemUploadBufferPtr = 0;

// ===== Initialize MMDVM Serial =====
void initModemSerial()
{
  MMDVM_SERIAL.begin(MMDVM_SERIAL_BAUD, SERIAL_8N1, MMDVM_RX_PIN, MMDVM_TX_PIN);
  delay(100);
  addLogMessage("[Modem] Serial2 initialized on GPIO " + String(MMDVM_TX_PIN) + "/" + String(MMDVM_RX_PIN));
}

// ===== Get MMDVM Modem Version =====
String getModemVersion()
{
  // MMDVM Protocol Constants
  const uint8_t MMDVM_FRAME_START = 0xE0;
  const uint8_t CMD_GET_VERSION = 0x00;

  // Initialize serial if needed
  if (!MMDVM_SERIAL)
  {
    initModemSerial();
  }

  // Clear RX buffer
  while (MMDVM_SERIAL.available())
  {
    MMDVM_SERIAL.read();
  }

  // Send GET_VERSION command
  uint8_t versionCmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};
  MMDVM_SERIAL.write(versionCmd, 3);
  MMDVM_SERIAL.flush();

  // Wait for response
  uint8_t rxBuffer[256];
  int rxPtr = 0;
  unsigned long timeout = millis() + 2000;

  while (millis() < timeout)
  {
    if (MMDVM_SERIAL.available())
    {
      uint8_t byte = MMDVM_SERIAL.read();
      if (rxPtr < 256)
      {
        rxBuffer[rxPtr++] = byte;

        // Check if we have a complete version response
        if (rxPtr >= 2)
        {
          uint8_t frameLength = rxBuffer[1];
          if (rxPtr >= frameLength && rxPtr >= 3 && rxBuffer[2] == CMD_GET_VERSION && rxPtr > 4)
          {
            // Extract version string
            String version = "";
            for (int i = 4; i < rxPtr && rxBuffer[i] != 0x00; i++)
            {
              if (rxBuffer[i] >= 32 && rxBuffer[i] < 127)
              {
                version += (char)rxBuffer[i];
              }
            }
            if (version.length() > 0)
            {
              addLogMessage("[Modem] Version: " + version);
              return version;
            }
          }
        }
      }
    }
    delay(10);
  }

  return "Not detected";
}

// ===== STM32 Bootloader Helper Functions =====

bool modemWaitForAck(const char *context)
{
  unsigned long timeout = millis() + 2000;
  while (millis() < timeout)
  {
    if (MMDVM_SERIAL.available())
    {
      uint8_t response = MMDVM_SERIAL.read();
      if (response == STM32_ACK)
      {
        return true;
      }
      else if (response == STM32_NACK)
      {
        addLogMessage("[Modem] NACK at " + String(context));
        return false;
      }
    }
    delay(10);
  }
  addLogMessage("[Modem] Timeout at " + String(context));
  return false;
}

bool modemSendCommand(uint8_t cmd)
{
  MMDVM_SERIAL.write(cmd);
  MMDVM_SERIAL.write(~cmd);
  MMDVM_SERIAL.flush();
  return modemWaitForAck("Command");
}

bool modemSyncBootloader()
{
  addLogMessage("[Modem] Syncing with STM32 bootloader...");

  // Clear RX buffer
  while (MMDVM_SERIAL.available())
  {
    MMDVM_SERIAL.read();
  }

  // Try multiple sync attempts with longer delays
  for (int attempt = 0; attempt < 10; attempt++)
  {
    // Clear RX buffer before each attempt
    while (MMDVM_SERIAL.available())
    {
      MMDVM_SERIAL.read();
    }

    MMDVM_SERIAL.write(STM32_SYNC_BYTE);
    MMDVM_SERIAL.flush();

    if (modemWaitForAck("Sync"))
    {
      addLogMessage("[Modem] Bootloader sync successful");
      return true;
    }
    delay(250);
  }

  addLogMessage("[Modem] ERROR: Bootloader sync failed");
  return false;
}

bool modemEraseFlash()
{
  addLogMessage("[Modem] Erasing flash memory...");

  if (!modemSendCommand(STM32_CMD_EE))
  {
    addLogMessage("[Modem] ERROR: Extended erase command failed");
    return false;
  }

  // Global erase: 0xFFFF
  MMDVM_SERIAL.write(0xFF);
  MMDVM_SERIAL.write(0xFF);
  MMDVM_SERIAL.write(0x00); // Checksum
  MMDVM_SERIAL.flush();

  // Wait for erase (can take 10-30 seconds)
  unsigned long timeout = millis() + 60000;
  while (millis() < timeout)
  {
    if (MMDVM_SERIAL.available())
    {
      uint8_t response = MMDVM_SERIAL.read();
      if (response == STM32_ACK)
      {
        addLogMessage("[Modem] Flash erase complete!");
        return true;
      }
      else if (response == STM32_NACK)
      {
        addLogMessage("[Modem] ERROR: Erase NACK received");
        return false;
      }
    }
    delay(100);
  }

  addLogMessage("[Modem] ERROR: Erase timeout");
  return false;
}

bool modemWriteMemory(uint32_t address, uint8_t *data, uint16_t length)
{
  if (!modemSendCommand(STM32_CMD_WM))
    return false;

  // Send address (4 bytes) + checksum
  uint8_t addrBytes[4] = {
      (uint8_t)(address >> 24),
      (uint8_t)(address >> 16),
      (uint8_t)(address >> 8),
      (uint8_t)address};

  uint8_t addrChecksum = addrBytes[0] ^ addrBytes[1] ^ addrBytes[2] ^ addrBytes[3];
  MMDVM_SERIAL.write(addrBytes, 4);
  MMDVM_SERIAL.write(addrChecksum);
  MMDVM_SERIAL.flush();

  if (!modemWaitForAck("Address"))
    return false;

  // Send length-1 (for 256 bytes, send 255)
  MMDVM_SERIAL.write((uint8_t)(length - 1));

  // Send data + checksum
  uint8_t dataChecksum = (uint8_t)(length - 1);
  for (uint16_t i = 0; i < length; i++)
  {
    MMDVM_SERIAL.write(data[i]);
    dataChecksum ^= data[i];
  }
  MMDVM_SERIAL.write(dataChecksum);
  MMDVM_SERIAL.flush();

  return modemWaitForAck("Write");
}

void modemEnterBootloader()
{
  addLogMessage("[Modem] Entering bootloader mode...");

  // Mark modem as not ready to stop protocol tasks from using it
  extern bool mmdvmReady;
  extern bool mmdvmWakeupActive;
  extern HardwareSerial MMDVMWakeup;
  extern TaskHandle_t dmrTaskHandle;
  extern TaskHandle_t dstarTaskHandle;
  extern TaskHandle_t ysfTaskHandle;
  extern TaskHandle_t p25TaskHandle;
  extern TaskHandle_t nxdnTaskHandle;
  extern TaskHandle_t pocsagTaskHandle;

  mmdvmReady = false;

  // Suspend all protocol tasks to ensure they stop using the serial port
  addLogMessage("[Modem] Suspending protocol tasks...");
  if (dmrTaskHandle != NULL)
  {
    vTaskSuspend(dmrTaskHandle);
    addLogMessage("[Modem] DMR task suspended");
  }
  if (dstarTaskHandle != NULL)
    vTaskSuspend(dstarTaskHandle);
  if (ysfTaskHandle != NULL)
    vTaskSuspend(ysfTaskHandle);
  if (p25TaskHandle != NULL)
    vTaskSuspend(p25TaskHandle);
  if (nxdnTaskHandle != NULL)
    vTaskSuspend(nxdnTaskHandle);
  if (pocsagTaskHandle != NULL)
    vTaskSuspend(pocsagTaskHandle);

  // Give some time for any in-flight serial operations to complete
  delay(200);

  // Initialize pins BEFORE stopping serials (GPIO 13 must be HIGH for normal operation)
  pinMode(MMDVM_BOOT_PIN, OUTPUT);
  digitalWrite(MMDVM_BOOT_PIN, LOW); // BOOT0 LOW for normal mode
  pinMode(MMDVM_RESET_PIN, OUTPUT);
  digitalWrite(MMDVM_RESET_PIN, HIGH); // RESET HIGH (not in reset)
  delay(100);

  // Stop wakeup serial (releases GPIO 13, but we already set it HIGH above)
  if (mmdvmWakeupActive)
  {
    addLogMessage("[Modem] Stopping wakeup serial...");
    MMDVMWakeup.end();
    mmdvmWakeupActive = false;
    delay(100);
    // Reconfigure GPIO 13 after serial.end() released it
    pinMode(MMDVM_RESET_PIN, OUTPUT);
    digitalWrite(MMDVM_RESET_PIN, HIGH);
  }

  // Stop main serial communication
  addLogMessage("[Modem] Stopping MMDVM serial...");
  MMDVM_SERIAL.end();
  delay(200);

  // Now enter bootloader mode with proper sequence
  addLogMessage("[Modem] Entering bootloader sequence...");

  // Set BOOT0 HIGH (must be high before reset)
  digitalWrite(MMDVM_BOOT_PIN, HIGH);
  delay(100);

  // Hold RESET LOW to reset the modem
  digitalWrite(MMDVM_RESET_PIN, LOW);
  delay(200);

  // Release RESET HIGH (modem boots with BOOT0=HIGH -> bootloader)
  digitalWrite(MMDVM_RESET_PIN, HIGH);
  delay(1000);

  // Restart serial with EVEN PARITY for bootloader (required by STM32 bootloader protocol)
  addLogMessage("[Modem] Starting bootloader serial...");
  MMDVM_SERIAL.begin(MMDVM_SERIAL_BAUD, SERIAL_8E1, MMDVM_RX_PIN, MMDVM_TX_PIN);
  delay(200);

  // Release RESET HIGH (modem boots with BOOT0=HIGH -> bootloader)
  digitalWrite(MMDVM_RESET_PIN, HIGH);
  delay(1000);

  // Restart serial with EVEN PARITY for bootloader (required by STM32 bootloader protocol)
  MMDVM_SERIAL.begin(MMDVM_SERIAL_BAUD, SERIAL_8E1, MMDVM_RX_PIN, MMDVM_TX_PIN);
  delay(200);

  // Clear RX buffer after serial restart
  while (MMDVM_SERIAL.available())
  {
    MMDVM_SERIAL.read();
  }

  modemInBootloaderMode = true;
  addLogMessage("[Modem] Bootloader mode active");
}

void modemExitBootloader()
{
  addLogMessage("[Modem] Exiting bootloader mode...");

  // Stop serial communication
  MMDVM_SERIAL.end();
  delay(200);

  // Set BOOT0 LOW (normal mode)
  digitalWrite(MMDVM_BOOT_PIN, LOW);
  delay(200);

  // Assert RESET LOW
  digitalWrite(MMDVM_RESET_PIN, LOW);
  delay(200);

  // Release RESET HIGH (modem boots with BOOT0=LOW -> normal firmware)
  digitalWrite(MMDVM_RESET_PIN, HIGH);
  delay(500);

  modemInBootloaderMode = false;

  addLogMessage("[Modem] Modem reset to normal mode");
}
