/*
 * dmr_network.h - DMR Network Protocol for ESP32 MMDVM Hotspot
 *
 * Handles DMR network connection, authentication, and keepalive
 *
 * NOTE: Function implementations remain in main .ino file
 * This header provides declarations and extern variables for DMR network functions
 */

#ifndef DMR_NETWORK_H
#define DMR_NETWORK_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include "config.h"

// DMR State Machine
enum class DMR_STATE {
  DISCONNECTED,
  WAITING_LOGIN,
  WAITING_AUTH,
  WAITING_CONFIG,
  CONNECTED
};

// External DMR network variables (defined in main .ino)
extern bool dmrLoggedIn;
extern String dmrLoginStatus;
extern DMR_STATE dmrState;
extern uint8_t dmrSalt[4];
extern unsigned long lastLoginAttempt;
extern int loginAttempts;
extern unsigned long lastKeepalive;

// External DMR configuration (defined in main .ino)
extern String dmr_callsign;
extern uint32_t dmr_id;
extern String dmr_server;
extern String dmr_password;
extern uint8_t dmr_essid;
extern uint32_t dmr_rx_freq;
extern uint32_t dmr_tx_freq;
extern uint8_t dmr_power;
extern uint8_t dmr_color_code;
extern float dmr_latitude;
extern float dmr_longitude;
extern int dmr_height;
extern String dmr_location;
extern String dmr_description;
extern String dmr_url;

// External UDP object
extern WiFiUDP udp;

// External logging functions
extern void logSerial(String message);
extern void logSerialVerbose(String message);

// External OLED update function
#if ENABLE_OLED
extern void updateOLEDStatus();
extern unsigned long lastOLEDUpdate;
#endif

// Function declarations
void connectToDMRNetwork();
void sendDMRAuth();
void sendDMRConfig();
void sendDMRKeepalive();
void handleNetwork();

#endif // DMR_NETWORK_H
