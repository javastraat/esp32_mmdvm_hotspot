/*
 * system_modem.h - MMDVM Modem Management
 *
 * Manages the MMDVM modem hardware communication:
 * - Wakeup serial (GPIO 13) - keeps modem active
 * - Communication serial (GPIO 43/44) - MMDVM protocol
 * - Initialization sequence
 * - Configuration management
 */

#ifndef SYSTEM_MODEM_H
#define SYSTEM_MODEM_H

#include <Arduino.h>
#include "../include/config.h"

// MMDVM Serial definitions
#define MMDVM_SERIAL Serial2
extern HardwareSerial MMDVMWakeup;

//CWID command (not part of original MMDVM protocol, but supported by modem firmware for sending CWID on demand)
#define CMD_SEND_CWID 0x0A

// MMDVM Protocol Constants
#define MMDVM_FRAME_START 0xE0

// MMDVM Commands
#define CMD_GET_VERSION 0x00
#define CMD_GET_STATUS 0x01
#define CMD_SET_CONFIG 0x02
#define CMD_SET_MODE 0x03
#define CMD_SET_FREQ 0x04
#define CMD_CAL_DATA 0x08
#define CMD_DMR_DATA1 0x18
#define CMD_DMR_LOST1 0x19
#define CMD_DMR_DATA2 0x1A
#define CMD_DMR_LOST2 0x1B
#define CMD_DMR_SHORTLC 0x1C
#define CMD_DMR_START 0x1D
#define CMD_DMR_ABORT 0x1E
#define CMD_POCSAG_DATA 0x50
#define CMD_ACK 0x70
#define CMD_NAK 0x7F

// Mode constants
#define MODE_IDLE 0U
#define MODE_DSTAR 1U
#define MODE_DMR 2U
#define MODE_YSF 3U
#define MODE_P25 4U
#define MODE_NXDN 5U
#define MODE_POCSAG 6U

// CW ID Settings (loaded from NVS)
extern bool cwidEnabled;
extern uint8_t cwidIntervalMin;

// Set to true to trigger an immediate CW ID (used by /api/test-cwid)
extern volatile bool cwidTestRequested;

// Modem state
extern bool mmdvmReady;
extern bool mmdvmWakeupActive;
extern String modemFirmwareVersion;

// Task handle
extern TaskHandle_t modemTaskHandle;

// Modem initialization and management
void initModemTask();
void modemTask(void *parameter);
void setupMMDVM();
void sendMMDVMCommand(uint8_t command, uint8_t *data, uint8_t dataLength);
void sendFrequency(uint32_t rxFreq, uint32_t txFreq, uint8_t rfPower);
bool waitForModemAck(unsigned long timeout);

#endif // SYSTEM_MODEM_H
