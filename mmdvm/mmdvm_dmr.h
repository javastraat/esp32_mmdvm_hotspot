/*
 * MMDVM DMR Protocol Module
 * Handles DMR (Digital Mobile Radio) protocol processing
 * and BrandMeister network integration
 */

#ifndef MMDVM_DMR_H
#define MMDVM_DMR_H

#include <Arduino.h>
#include "../include/config.h"

// DMR Network State Machine
enum class DMR_STATE
{
  DISCONNECTED,
  WAITING_LOGIN,
  WAITING_AUTH,
  WAITING_CONFIG,
  CONNECTED
};

// DMR network state (readable by other tasks, e.g. web/OLED)
extern volatile DMR_STATE dmrState;
extern volatile bool dmrLoggedIn;
extern String dmrLoginStatus;

// Task handle
extern TaskHandle_t dmrTaskHandle;

// Task init and entry points
void initDmrTask();
void dmrTask(void *parameter);
void initDmrLookupTask();
void dmrLookupTask(void *parameter);

// DMR user info cache
String getCachedDmrUserInfo(uint32_t dmrId);

// DMR network protocol functions
void connectToDMRNetwork();
void sendDMRAuth(const uint8_t *salt);
void sendDMRConfig();
void sendDMRKeepalive();
void handleDMRNetwork();

// DMR modem control
void writeDMRStart(bool tx);

// DMR TX state (true when modem is actively transmitting DMR to RF)
extern volatile bool dmrTxActive;

// DMR RF RX state (true while modem is receiving a DMR transmission from RF)
extern volatile bool dmrRfRxActive;

#endif // MMDVM_DMR_H
