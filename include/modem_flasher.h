/*
 * modem_flasher.h - MMDVM Modem Firmware Flasher
 *
 * STM32 bootloader protocol implementation for RTOS version
 * Allows flashing MMDVM modem firmware via web interface
 */

#ifndef MODEM_FLASHER_H
#define MODEM_FLASHER_H

#include <Arduino.h>
#include "config.h"

// Serial2 for MMDVM communication
#define MMDVM_SERIAL Serial2

// STM32 Bootloader Protocol Constants
#define STM32_SYNC_BYTE 0x7F
#define STM32_ACK 0x79
#define STM32_NACK 0x1F

// STM32 Commands
#define STM32_CMD_GET 0x00
#define STM32_CMD_GID 0x02
#define STM32_CMD_RM 0x11
#define STM32_CMD_WM 0x31
#define STM32_CMD_EE 0x44

// Flash Memory
#define FLASH_START_ADDR 0x08000000
#define MAX_WRITE_SIZE 256

// Global state for modem flasher
extern bool modemInBootloaderMode;
extern bool modemFlashInProgress;
extern int modemFlashProgress; // 0-100 percentage
extern String modemFlashStatus;
extern uint8_t* modemFirmwareBuffer;
extern size_t modemFirmwareSize;

// Function declarations
void initModemSerial();
String getModemVersion();
bool modemWaitForAck(const char *context);
bool modemSendCommand(uint8_t cmd);
bool modemSyncBootloader();
bool modemEraseFlash();
bool modemWriteMemory(uint32_t address, uint8_t *data, uint16_t length);
void modemEnterBootloader();
void modemExitBootloader();

#endif // MODEM_FLASHER_H
