/*
 * mmdvm_modem.h - MMDVM Modem Hardware Communication for ESP32 MMDVM Hotspot
 *
 * Handles communication with MMDVM modem hardware via serial
 *
 * NOTE: Function implementations remain in main .ino file
 * This header provides declarations and extern variables for MMDVM modem functions
 */

#ifndef MMDVM_MODEM_H
#define MMDVM_MODEM_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "config.h"

// MMDVM Command Definitions
#define MMDVM_FRAME_START    0xE0
#define CMD_GET_VERSION      0x00
#define CMD_GET_STATUS       0x01
#define CMD_SET_CONFIG       0x02
#define CMD_SET_MODE         0x03
#define CMD_SET_FREQ         0x04
#define CMD_DMR_DATA1        0x18
#define CMD_DMR_DATA2        0x19
#define CMD_DMR_START        0x1A
#define CMD_DMR_SHORTLC      0x1B
#define CMD_DMR_ABORT        0x1C

// MMDVM Mode Bits
#define MODE_IDLE    0x00
#define MODE_DSTAR   0x01
#define MODE_DMR     0x02
#define MODE_YSF     0x04
#define MODE_P25     0x08
#define MODE_NXDN    0x10
#define MODE_POCSAG  0x20

// External modem state variables (defined in main .ino)
extern bool mmdvmReady;
extern bool mmdvmWakeupActive;
extern bool dmrTxActive;
extern String modemFirmwareVersion;

// External serial objects (defined in main .ino)
extern HardwareSerial MMDVM_SERIAL;
extern HardwareSerial MMDVMWakeup;

// External buffers
extern uint8_t rxBuffer[512];
extern uint8_t txBuffer[512];
extern int rxBufferPtr;

// External configuration
extern uint32_t dmr_rx_freq;
extern uint32_t dmr_tx_freq;
extern uint8_t dmr_power;
extern uint8_t dmr_color_code;

// External mode enable flags
extern bool mode_dmr_enabled;
extern bool mode_dstar_enabled;
extern bool mode_ysf_enabled;
extern bool mode_p25_enabled;
extern bool mode_nxdn_enabled;
extern bool mode_pocsag_enabled;

// External logging function
extern void logSerial(String message);

// Function declarations
void setupMMDVM();
void handleMMDVMSerial();
void sendMMDVMCommand(uint8_t cmd, uint8_t* data, uint16_t length);
void processMMDVMFrame();
void sendFrequency(uint32_t rxFreq, uint32_t txFreq, uint8_t rfPower);
void writeDMRStart(bool tx, String callsign = "");

#endif // MMDVM_MODEM_H
