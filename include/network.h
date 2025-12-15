/*
 * network.h - Network Connectivity for ESP32 MMDVM Hotspot
 *
 * Handles WiFi, Ethernet, and Access Point connectivity
 *
 * NOTE: Function implementations remain in main .ino file
 * This header provides declarations and extern variables for network functions
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
#include <ETH.h>
#endif
#include "config.h"

// External network state variables (defined in main .ino)
extern bool wifiConnected;
extern bool apMode;
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
extern bool eth_connected;
#endif

// External WiFi credentials (defined in main .ino)
extern const char* ssid;
extern const char* password;
extern const char* ap_ssid;
extern const char* ap_password;

// External WiFi networks array
struct WiFiNetwork;  // Forward declaration
extern WiFiNetwork wifiNetworks[5];

// External device hostname
extern String device_hostname;

// External UDP object
extern WiFiUDP udp;

// External LED control functions
extern void setLEDMode(LED_MODE mode);
extern void updateStatusLED();

// External logging function
extern void logSerial(String message);

// Function declarations
void setupWiFi();
void setupAccessPoint();
void setupEthernet();
void WiFiEvent(arduino_event_id_t event);

#endif // NETWORK_H
