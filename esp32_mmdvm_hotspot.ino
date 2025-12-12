/*
 * ESP32 MMDVM Hotspot 
 *
 * This code provides a basic framework for using an ESP32 with an MMDVM hat
 * to create a DMR hotspot similar to Pi-Star.
 *
 * Hardware Requirements:
 * - ESP32 Development Board
 * - MMDVM Hat (compatible with ESP32 3.3V logic or use level shifters)
 * - Antenna and appropriate radio frontend
 *
 * Connections:
 * - MMDVM RX -> ESP32 GPIO16 (RX2)
 * - MMDVM TX -> ESP32 GPIO17 (TX2)
 * - MMDVM PTT -> ESP32 GPIO4
 * - MMDVM COSLED -> ESP32 GPIO2
 *
 * Libraries needed:
 * - WiFi (built-in)
 * - WiFiUDP (built-in)
 * - WebServer (built-in)
 * - ESPmDNS (built-in)
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "mbedtls/md.h"
#include "nvs_flash.h"
#include <Update.h>
#include <HTTPClient.h>
#include <time.h>
#include "config.h"
#include "webpages.h"
#include "RGBLedController.h"

// OLED Display Support
#if ENABLE_OLED
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#endif

#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
#if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(3,0,0)
#include <ETHClass2.h>       //Is to use the modified ETHClass
#define ETH  ETH2
#else
#include <ETH.h>
#endif
static bool eth_connected = false;
#include <SPI.h>
#include <SD.h>
#endif  // LILYGO_T_ETH_ELITE_ESP32S3_MMDVM

// SD Card pins are defined in config.h
SPIClass sdSPI(HSPI);  // Use HSPI for SD card
bool sdCardAvailable = false;
uint8_t sdCardType = 0;  // Cached SD card type

// ESP32-S3 USB Serial configuration
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
#if ARDUINO_USB_MODE
#warning "USB MODE is enabled - Serial will work via USB CDC"
#endif
#if ARDUINO_USB_CDC_ON_BOOT
#warning "USB CDC ON BOOT is enabled"
#endif
#endif

// ===== Configuration from config.h =====
// WiFi Settings
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Fallback AP mode settings (from config.h)
const char* ap_ssid = AP_SSID;
const char* ap_password = AP_PASSWORD;

// DMR Network Settings (can be overridden by stored config)
String dmr_callsign = DMR_CALLSIGN;
uint32_t dmr_id = DMR_ID;
String dmr_server = DMR_SERVER;
String dmr_password = DMR_PASSWORD;
uint8_t dmr_essid = 0;  // ESSID 0-99
const int dmr_port = DMR_PORT;

// Additional DMR Configuration
uint32_t dmr_rx_freq = 434000000;  // RX Frequency in Hz
uint32_t dmr_tx_freq = 434000000;  // TX Frequency in Hz
uint8_t dmr_power = 10;            // Power 0-99
uint8_t dmr_color_code = 1;        // Color Code 0-15
float dmr_latitude = 0.0;          // Latitude
float dmr_longitude = 0.0;         // Longitude
int dmr_height = 0;                // Height in meters
String dmr_location = DMR_LOCATION;
String dmr_description = DMR_DESCRIPTION;
String dmr_url = DMR_URL;

// Hostname setting
String device_hostname = MDNS_HOSTNAME;

// NTP Time settings
long ntp_timezone_offset = NTP_TIMEZONE_OFFSET;    // Timezone offset in seconds
long ntp_daylight_offset = NTP_DAYLIGHT_OFFSET;    // Daylight saving offset in seconds

// Verbose logging setting (shows keepalive messages)
bool verbose_logging = false;

// Web interface credentials (from config.h)
String web_username = WEB_USERNAME;
String web_password = WEB_PASSWORD;

// MMDVM Settings
#define SERIAL_BAUD MMDVM_SERIAL_BAUD
#define MMDVM_SERIAL Serial2
#define PTT_PIN MMDVM_PTT_PIN
#define COS_LED_PIN MMDVM_COS_LED_PIN
#define RX_PIN MMDVM_RX_PIN
#define TX_PIN MMDVM_TX_PIN

// MMDVM Wakeup Serial (GPIO 13 keeps modem active)
HardwareSerial MMDVMWakeup(1);
bool mmdvmWakeupActive = false;

// ===== Protocol Constants =====
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
#define CMD_ACK 0x70
#define CMD_NAK 0x7F

// DMR Protocol
#define DMR_SLOT1 0x00
#define DMR_SLOT2 0x01

// ===== Global Variables =====
WiFiUDP udp;
WebServer server(80);
Preferences preferences;
bool wifiConnected = false;
bool apMode = false;
bool mmdvmReady = false;
bool dmrLoggedIn = false;
uint32_t currentTalkgroup = 0;
String dmrLoginStatus = "Not Connected";
uint8_t rxBuffer[512];
uint8_t txBuffer[512];
int rxBufferPtr = 0;

// DMR Network State Machine
enum class DMR_STATE {
  DISCONNECTED,
  WAITING_LOGIN,
  WAITING_AUTH,
  WAITING_CONFIG,
  CONNECTED
};
DMR_STATE dmrState = DMR_STATE::DISCONNECTED;
uint8_t dmrSalt[4];
unsigned long lastLoginAttempt = 0;
int loginAttempts = 0;

// RGB LED Status Indicator
#if ENABLE_RGB_LED
RGBLedController rgbLed(LEDBORG_RED_PIN, LEDBORG_GREEN_PIN, LEDBORG_BLUE_PIN,
                        RGB_LED_IDLE_BRIGHTNESS, RGB_LED_ACTIVE_BRIGHTNESS);
#endif

// OLED Display
#if ENABLE_OLED
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
SemaphoreHandle_t displayMutex = NULL;  // Mutex to protect display access from SPI conflicts
unsigned long lastOLEDUpdate = 0;
unsigned long lastNetworkToggle = 0;     // Separate timer for network toggle
#define OLED_UPDATE_INTERVAL 5000        // Update OLED every 5 seconds (idle)
#define OLED_UPDATE_INTERVAL_ACTIVE 1000 // Update OLED every 2 seconds when DMR active (reduced from 1s to avoid SPI conflicts)
#define NETWORK_TOGGLE_INTERVAL 5000     // Toggle network display every 5 seconds
bool oledShowEthernet = true;  // Toggle between ETH and WiFi display when both connected
int oledActiveSlot = 1;        // Track which slot to display when both active (start with Slot 2)
int oledHeaderCycle = 0;       // Cycle through: 0=WiFi, 1=ETH, 2=Callsign (or fewer if not all connected)
//
// #define LOGO_HEIGHT   16
// #define LOGO_WIDTH    16
static const unsigned char PROGMEM logo_bmp1[] =
{ 
  B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 

};

#define LOGO_WIDTH  128
#define LOGO_HEIGHT 64
static const unsigned char PROGMEM logo_bmp[] =
{
// 'p', 128x64px
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x1f, 0xfc, 0x1e, 0x9f, 0xf0, 0x3c, 0x3c, 0x03, 0xf3, 0xf0, 0x00, 0x40, 0x00, 0x00, 0x00, 0x20, 
0x07, 0x0c, 0x31, 0x87, 0x38, 0x7e, 0x7e, 0x00, 0xe1, 0xc0, 0x00, 0x40, 0x00, 0x00, 0x00, 0x20, 
0x07, 0x04, 0x70, 0x87, 0x1c, 0x8e, 0x4f, 0x00, 0xe1, 0xc0, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x60, 
0x07, 0x20, 0x70, 0x87, 0x1c, 0x0c, 0x07, 0x00, 0xe1, 0xc0, 0xf3, 0xf3, 0xbe, 0xc0, 0x79, 0xf8, 
0x07, 0x20, 0x78, 0x07, 0x1c, 0x18, 0x07, 0x00, 0xe1, 0xc1, 0x99, 0xc6, 0x6f, 0x60, 0xcc, 0xe0, 
0x07, 0xe0, 0x3e, 0x07, 0x38, 0x3c, 0x06, 0x00, 0xff, 0xc3, 0x9d, 0xc7, 0x2e, 0x71, 0xce, 0xe0, 
0x07, 0x20, 0x0f, 0x87, 0xf0, 0x1e, 0x0c, 0x00, 0xe1, 0xc3, 0x9d, 0xc7, 0x8e, 0x71, 0xce, 0xe0, 
0x07, 0x20, 0x43, 0xc7, 0x00, 0x0f, 0x08, 0x00, 0xe1, 0xc3, 0x9d, 0xc3, 0xce, 0x71, 0xce, 0xe0, 
0x07, 0x02, 0x41, 0xc7, 0x00, 0x07, 0x10, 0x00, 0xe1, 0xc3, 0x9d, 0xc1, 0xee, 0x71, 0xce, 0xe0, 
0x07, 0x02, 0x61, 0xc7, 0x00, 0xc7, 0x20, 0x80, 0xe1, 0xc3, 0x9d, 0xc4, 0xee, 0x71, 0xce, 0xe0, 
0x07, 0x06, 0x71, 0x87, 0x00, 0xe6, 0x7f, 0x00, 0xe1, 0xc1, 0x99, 0xd6, 0x6f, 0x60, 0xcc, 0xe8, 
0x1f, 0xfe, 0x4f, 0x1f, 0xc0, 0x78, 0xff, 0x03, 0xf3, 0xf0, 0xf0, 0xe5, 0xce, 0xc0, 0x78, 0x70, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x38, 0x00, 0x1f, 0xbf, 0x0e, 0x7f, 0x70, 0xf1, 0xd0, 0x1c, 0x07, 0xef, 0xc3, 0x87, 0x8f, 0x00, 
0x18, 0x00, 0x0c, 0xd9, 0x8f, 0x31, 0x38, 0xe3, 0x30, 0x32, 0x03, 0x36, 0x66, 0xc3, 0x19, 0x80, 
0x18, 0x00, 0x0c, 0xd8, 0xd3, 0x30, 0x38, 0xe6, 0x10, 0x32, 0x03, 0x36, 0x36, 0xc3, 0x30, 0xc0, 
0x1e, 0x3b, 0x0c, 0xd8, 0xc3, 0x34, 0x3d, 0x66, 0x00, 0x34, 0x03, 0x36, 0x36, 0xc3, 0x30, 0xc0, 
0x1b, 0x1a, 0x0f, 0x98, 0xc2, 0x3c, 0x2d, 0x66, 0x00, 0x5b, 0x83, 0xe6, 0x33, 0x83, 0x30, 0xc0, 
0x1b, 0x1a, 0x0c, 0x18, 0xc4, 0x34, 0x2d, 0x66, 0x00, 0xd9, 0x03, 0x06, 0x36, 0xc3, 0x30, 0xc0, 
0x1b, 0x0c, 0x0c, 0x18, 0xc8, 0x30, 0x2e, 0x66, 0x00, 0xce, 0x03, 0x06, 0x36, 0xdb, 0x30, 0xc0, 
0x1b, 0x0c, 0x0c, 0x19, 0x8f, 0x31, 0x26, 0x63, 0x10, 0xe6, 0x83, 0x06, 0x66, 0xdb, 0x19, 0x80, 
0x16, 0x04, 0x1e, 0x3f, 0x1f, 0x7f, 0x76, 0xf1, 0xe0, 0x7b, 0x07, 0x8f, 0xc3, 0x8e, 0x0f, 0x00, 
0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Icon bitmaps for OLED display (8x8 pixels)
#define ICON_WIDTH 8
#define ICON_HEIGHT 8

// WiFi icon (8x8)
static const unsigned char PROGMEM icon_wifi[] = {
  0b00000000,
  0b00111100,
  0b01000010,
  0b10011001,
  0b00100100,
  0b00011000,
  0b00011000,
  0b00000000
};

// Ethernet/Cable icon (8x8)
static const unsigned char PROGMEM icon_ethernet[] = {
  0b00000000,
  0b01111110,
  0b01000010,
  0b01011010,
  0b01011010,
  0b01000010,
  0b01111110,
  0b00000000
};

// Antenna/RF icon (8x8) - looks like radio waves
static const unsigned char PROGMEM icon_antenna[] = {
  0b00011000,
  0b00111100,
  0b01011010,
  0b10011001,
  0b00011000,
  0b00100100,
  0b01000010,
  0b10000001
};

// DMR icon (8x8)
static const unsigned char PROGMEM icon_dmr[] = {
  0b11111111,
  0b11000011,
  0b10100101,
  0b10011001,
  0b10011001,
  0b10100101,
  0b11000011,
  0b11111111
};

#endif

// Serial Monitor Buffer (SERIAL_LOG_SIZE defined in webpages.h)
String serialLog[SERIAL_LOG_SIZE];
int serialLogIndex = 0;

unsigned long lastKeepalive = 0;

// DMR Activity Tracking (struct defined in home.h)
// Track up to 2 simultaneous transmissions (one per slot)
DMRActivity dmrActivity[2] = {
  {0, 0, 1, true, "", "", "", "", "", 0, 0, false},
  {0, 0, 2, true, "", "", "", "", "", 0, 0, false}
};

// DMR Transmission Tracking (for consolidated log output)
struct DMRTransmission {
  uint32_t srcId;
  uint32_t dstId;
  uint8_t slotNo;
  bool isGroup;
  uint8_t startSeq;
  uint8_t lastSeq;
  bool active;
  String frameType;
};
DMRTransmission currentTx[2] = {{0, 0, 0, true, 0, 0, false, ""}, {0, 0, 0, true, 0, 0, false, ""}};

// DMR Transmission History (for Recent Activity display)
// struct DMRHistory is defined in webpages.h/home.h
DMRHistory dmrHistory[DMR_HISTORY_SIZE];
int dmrHistoryIndex = 0;
void addDMRHistory(uint32_t srcId, String srcCallsign, String srcName, String srcLocation, uint32_t dstId, bool isGroup, uint32_t duration, uint8_t ber, uint8_t rssi, uint8_t slotNo);

// DMR User Information Lookup Cache
struct UserInfoCache {
  uint32_t dmrId;
  String callsign;
  String userInfo;  // Format: "callsign|name|city|country"
  unsigned long timestamp;
};
UserInfoCache userCache[DMR_USER_CACHE_SIZE];
int userCacheIndex = 0;

// Legacy callsign cache for backward compatibility
struct CallsignCache {
  uint32_t dmrId;
  String callsign;
  unsigned long timestamp;
};
CallsignCache callsignCache[DMR_CALLSIGN_CACHE_SIZE];
int callsignCacheIndex = 0;

// Store alternate WiFi credentials
// Alternate WiFi Networks (up to 5) - labels from config.h
WiFiNetwork wifiNetworks[5] = {
  { WIFI_SLOT1_LABEL, "", "" },
  { WIFI_SLOT2_LABEL, "", "" },
  { WIFI_SLOT3_LABEL, "", "" },
  { WIFI_SLOT4_LABEL, "", "" },
  { WIFI_SLOT5_LABEL, "", "" }
};

// Firmware version from config.h
String firmwareVersion = FIRMWARE_VERSION;
String modemFirmwareVersion = "Unknown"; // MMDVM modem firmware version

// LED Status Control (STATUS_LED_PIN defined in config.h)

enum class LED_MODE {
  OFF,
  STEADY,      // Connected to WiFi
  FAST_BLINK,  // Connecting to WiFi
  SLOW_BLINK   // Access Point mode
};
LED_MODE currentLedMode = LED_MODE::OFF;
unsigned long lastLedToggle = 0;
bool ledState = false;

// Mode Enable/Disable Settings (defaults from config.h)
bool mode_dmr_enabled = DEFAULT_MODE_DMR;
bool mode_dstar_enabled = DEFAULT_MODE_DSTAR;
bool mode_ysf_enabled = DEFAULT_MODE_YSF;
bool mode_p25_enabled = DEFAULT_MODE_P25;
bool mode_nxdn_enabled = DEFAULT_MODE_NXDN;
bool mode_pocsag_enabled = DEFAULT_MODE_POCSAG;

// Modem Type Selection
String modem_type = DEFAULT_MODEM_TYPE;

// ===== Function Prototypes =====
void setupWiFi();
void setupAccessPoint();
void setupEthernet();
void WiFiEvent(arduino_event_id_t event);
void setupWebServer();
void setupMMDVM();
void setupOLED();
void displayESP32Logo();
void testdrawbitmap();
void displayBootLogo();
void updateBootStatus(String status);
void updateOLEDStatus();
void loadConfig();
void saveConfig();
void handleMMDVMSerial();
void handleNetwork();
void sendMMDVMCommand(uint8_t cmd, uint8_t* data, uint16_t length);
void writeDMRStart(bool tx);
void sendFrequency(uint32_t rxFreq, uint32_t txFreq, uint8_t rfPower);
void processMMDVMFrame();
void updateStatusLED();
void setLEDMode(LED_MODE mode);
void sendDMRKeepalive();
void connectToDMRNetwork();
void sendDMRAuth();
void sendDMRConfig();
void logSerial(String message);
void logSerialVerbose(String message);
String lookupCallsign(uint32_t dmrId);
String lookupCallsignAPI(uint32_t dmrId);
String lookupUserInfo(uint32_t dmrId);
String lookupUserInfoAPI(uint32_t dmrId);
String getCachedCallsign(uint32_t dmrId);
String getCachedUserInfo(uint32_t dmrId);
void cacheCallsign(uint32_t dmrId, String callsign);
void cacheUserInfo(uint32_t dmrId, String userInfo);
void addDMRHistory(uint32_t srcId, String srcCallsign, String srcName, String srcLocation, uint32_t dstId, bool isGroup, uint32_t duration, uint8_t ber, uint8_t rssi, uint8_t slotNo);

#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
// Helper functions for status page
String getEthIPAddress();
String getEthMACAddress();
int getEthLinkSpeed();
bool getEthFullDuplex();
String getEthGatewayIP();
uint64_t getSDCardSize();
uint64_t getSDUsedBytes();
uint8_t getSDCardType();
#endif

// Web handlers are defined in webpages.h

void setup() {
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
  // ESP32-S3 USB CDC (native USB support)
  Serial.begin(115200);
  delay(1000);
  // Wait for USB Serial to be ready (ESP32-S3 native USB)
  while (!Serial && millis() < 3000) {
    delay(10);
  }
#else
  Serial.begin(115200);
  delay(1000);
#endif

  logSerial("\n\n=== ESP32 MMDVM Hotspot ===");
  logSerial("Initializing...");

  // Initialize OLED Display early (before other components)
  // Load saved configuration

#if ENABLE_OLED
  setupOLED();
  updateBootStatus("Loading config...");
#endif

  loadConfig();

  // Setup GPIO
  pinMode(PTT_PIN, OUTPUT);
  pinMode(COS_LED_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);  // Setup status LED
  digitalWrite(PTT_PIN, LOW);
  digitalWrite(COS_LED_PIN, LOW);
  digitalWrite(STATUS_LED_PIN, LOW);

  // Initialize RGB LED Status Indicator
#if ENABLE_RGB_LED
  rgbLed.begin();
  rgbLed.setStatus(RGBLedStatus::DISCONNECTED);
  logSerial("RGB LED initialized");
#endif

  // Setup MMDVM Serial
#if ENABLE_OLED
  updateBootStatus("Init MMDVM...");
#endif
  MMDVM_SERIAL.begin(SERIAL_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  logSerial("MMDVM Serial initialized");

  // Initialize SD Card
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
#if ENABLE_OLED
  updateBootStatus("Init SD Card...");
#endif
  logSerial("Initializing SD card...");
  sdSPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

  if (SD.begin(SD_CS_PIN, sdSPI)) {
    sdCardAvailable = true;
    sdCardType = SD.cardType();  // Cache the card type
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    logSerial("SD Card initialized successfully");
    logSerial("SD Card Size: " + String((uint32_t)cardSize) + " MB");
    logSerial("SD Card Type: " + String(sdCardType == CARD_MMC ? "MMC" : sdCardType == CARD_SD ? "SD" : sdCardType == CARD_SDHC ? "SDHC" : "Unknown"));

    // Create directories if they don't exist
    if (!SD.exists("/logs")) {
      SD.mkdir("/logs");
      logSerial("Created /logs directory");
    }
    if (!SD.exists("/config")) {
      SD.mkdir("/config");
      logSerial("Created /config directory");
    }
    if (!SD.exists("/cache")) {
      SD.mkdir("/cache");
      logSerial("Created /cache directory");
    }

    // Test: Read owner.txt if it exists
    if (SD.exists("/owner.txt")) {
      logSerial("Found /owner.txt on SD card, reading contents:");
      File testFile = SD.open("/owner.txt");
      if (testFile) {
        logSerial("--- Start of owner.txt ---");
        while (testFile.available()) {
          Serial.write(testFile.read());
        }
        testFile.close();
        logSerial("\n--- End of owner.txt ---");
      } else {
        logSerial("Error: Could not open owner.txt");
      }
    } else {
      logSerial("No owner.txt file found on SD card");
    }
  } else {
    sdCardAvailable = false;
    logSerial("SD Card initialization failed (card not present or error)");
    logSerial("Continuing without SD card support...");
  }
#endif

  // Setup Network (Ethernet with WiFi fallback, or WiFi only)
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
#if ENABLE_OLED
    updateBootStatus("Init Ethernet...");
#endif
    setupEthernet();

  // Wait up to 10 seconds for Ethernet to connect
  logSerial("Waiting for Ethernet connection...");
  int eth_attempts = 0;
  while (!eth_connected && eth_attempts < 20) {
    delay(500);
    Serial.print(".");
    eth_attempts++;
  }
  
  if (eth_connected) {
    logSerial("\nEthernet connected successfully!");
    logSerial("IP address: " + ETH.localIP().toString());
  } else {
    // Ethernet failed, try WiFi (Ethernet will keep trying in background)
    logSerial("\nEthernet connection timeout. Falling back to WiFi...");
    logSerial("Note: Ethernet will continue trying in background.");
#if ENABLE_OLED
    updateBootStatus("Connecting WiFi...");
#endif  
    setupWiFi();
  }
#else
  #if ENABLE_OLED 
  updateBootStatus("Connecting WiFi...");
  #endif
  setupWiFi();
#endif

// Setup Web Server
//setupWebServer();
#if ENABLE_WEBSERVER
#if ENABLE_OLED
  updateBootStatus("Starting web...");
#endif
  setupWebServer();
#endif

// Start mDNS
// if (MDNS.begin(device_hostname.c_str())) {
//   logSerial("mDNS started: http://" + device_hostname + ".local");
// }
#if ENABLE_MDNS
  if (MDNS.begin(device_hostname.c_str())) {
    logSerial("mDNS started: http://" + device_hostname + ".local");
  }
#endif

  // Initialize NTP Time
  // Check if we have any network connection
  bool hasNetwork = wifiConnected;
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
  hasNetwork = hasNetwork || eth_connected;
#endif

  if (hasNetwork) {
#if ENABLE_OLED
    updateBootStatus("Syncing time...");
#endif
    logSerial("Initializing NTP time client...");
    // Configure time with NTP servers (using saved timezone settings or config.h defaults)
    configTime(ntp_timezone_offset, ntp_daylight_offset, NTP_SERVER1, NTP_SERVER2);

    // Wait a bit for time sync
    int ntp_attempts = 0;
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo) && ntp_attempts < 10) {
      delay(500);
      ntp_attempts++;
    }

    if (ntp_attempts < 10) {
      logSerial("NTP time synchronized successfully");
      char timeStr[64];
      strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
      logSerial("Current time: " + String(timeStr));
    } else {
      logSerial("Warning: NTP time sync failed, timestamps will use system time");
    }
  }

  // Initialize MMDVM
  setupMMDVM();

  // Connect to Networks (based on enabled modes)
  if (wifiConnected) {
    // DMR Network Connection
    if (mode_dmr_enabled) {
      #if ENABLE_OLED
      updateBootStatus("Connecting DMR...");
      #endif
      connectToDMRNetwork();
    } else {
      logSerial("DMR mode is disabled - skipping DMR network connection");
    }

    // D-Star Network Connection (not implemented yet)
    if (mode_dstar_enabled) {
      logSerial("D-Star mode is enabled but network connection not implemented yet");
    } else {
      logSerial("D-Star mode is disabled by default (not implemented yet)");
    }

    // YSF Network Connection (not implemented yet)
    if (mode_ysf_enabled) {
      logSerial("YSF mode is enabled but network connection not implemented yet");
    } else {
      logSerial("YSF mode is disabled by default (not implemented yet)");
    }

    // P25 Network Connection (not implemented yet)
    if (mode_p25_enabled) {
      logSerial("P25 mode is enabled but network connection not implemented yet");
    } else {
      logSerial("P25 mode is disabled by default (not implemented yet)");
    }

    // NXDN Network Connection (not implemented yet)
    if (mode_nxdn_enabled) {
      logSerial("NXDN mode is enabled but network connection not implemented yet");
    } else {
      logSerial("NXDN mode is disabled by default (not implemented yet)");
    }

    // POCSAG Network Connection (not implemented yet)
    if (mode_pocsag_enabled) {
      logSerial("POCSAG mode is enabled but network connection not implemented yet");
    } else {
      logSerial("POCSAG mode is disabled by default (not implemented yet)");
    }
  } else {
    logSerial("WiFi not connected - skipping all network connections");
  }

#if ENABLE_OLED
  updateBootStatus("Ready!");
#endif
  delay(1000);  // Show "Ready!" for 1 second

  logSerial("Setup complete!");
  if (apMode) {
    logSerial("Access Point Mode - Connect to: " + String(ap_ssid));
    logSerial("Web Interface: http://192.168.4.1");
  } else if (wifiConnected) {
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
    if (eth_connected) {
      logSerial("Web Interface: http://" + ETH.localIP().toString());
    } else {
      logSerial("Web Interface: http://" + WiFi.localIP().toString());
    }
#else
    logSerial("Web Interface: http://" + WiFi.localIP().toString());
#endif
  }

  // Update OLED with network status after setup is complete
#if ENABLE_OLED
  updateOLEDStatus();
#endif
}

void loop() {
  // MMDVM wakeup serial on GPIO 13 stays open (no need to send more data after initial wakeup)
  // Just keeping the UART port active is enough to keep the modem awake
  
  // Update status LED
  updateStatusLED();

  // Update RGB LED (for blinking effects)
#if ENABLE_RGB_LED
  rgbLed.update();
#endif

  // Handle web server
  server.handleClient();

  // Handle MMDVM serial communication
  handleMMDVMSerial();

  // Check for DMR activity timeout and update OLED display
  unsigned long currentMillis = millis();

  // Handle network toggle on separate timer (always 5 seconds)
#if ENABLE_OLED
  if (currentMillis - lastNetworkToggle >= NETWORK_TOGGLE_INTERVAL) {
    // Cycle through header display states
    // Determine max cycle value based on connections
    bool hasWifi = wifiConnected;
    bool hasEth = false;
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
    bool hasEth = false;
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
    hasEth = eth_connected;
#endif
#endif

    int maxCycle = 1; // Default: 2 states (network + callsign)
    if (hasWifi && hasEth) {
      maxCycle = 2; // 3 states (wifi, eth, callsign)
    }

    oledHeaderCycle++;
    if (oledHeaderCycle > maxCycle) {
      oledHeaderCycle = 0;
    }

    // Keep old toggle for backward compatibility if needed
    oledShowEthernet = !oledShowEthernet;
    lastNetworkToggle = currentMillis;
  }
#endif

  // Update OLED display periodically (faster when DMR activity is active)
#if ENABLE_OLED
  // Check if there's any active DMR transmission
  bool anyDMRActive = dmrActivity[0].active || dmrActivity[1].active;
  unsigned long oledInterval = anyDMRActive ? OLED_UPDATE_INTERVAL_ACTIVE : OLED_UPDATE_INTERVAL;

  if (currentMillis - lastOLEDUpdate >= oledInterval) {
    updateOLEDStatus();
    lastOLEDUpdate = currentMillis;
  }
#endif
  for (int i = 0; i < 2; i++) {
    // Check if DMR activity has timed out and add to history before deactivating
    if (dmrActivity[i].active && (currentMillis - dmrActivity[i].lastUpdate > DMR_ACTIVITY_TIMEOUT)) {
      // Add to history when activity times out (transmission ended)
      if (dmrActivity[i].srcId > 0) {
        uint32_t duration = (currentMillis - dmrActivity[i].startTime) / 1000;
        String location = "";
        if (dmrActivity[i].srcCity.length() > 0 || dmrActivity[i].srcCountry.length() > 0) {
          if (dmrActivity[i].srcCity.length() > 0) location += dmrActivity[i].srcCity;
          if (dmrActivity[i].srcCity.length() > 0 && dmrActivity[i].srcCountry.length() > 0) location += ", ";
          if (dmrActivity[i].srcCountry.length() > 0) location += dmrActivity[i].srcCountry;
        }
        addDMRHistory(dmrActivity[i].srcId, dmrActivity[i].srcCallsign, dmrActivity[i].srcName, location,
                     dmrActivity[i].dstId, dmrActivity[i].isGroup, duration, 0, 0, dmrActivity[i].slotNo);
      }
      dmrActivity[i].active = false;
    }
    
    // Also check transmission timeout and log the end
    if (currentTx[i].active && (currentMillis - dmrActivity[i].lastUpdate > DMR_ACTIVITY_TIMEOUT)) {
      if (currentTx[i].lastSeq > currentTx[i].startSeq) {
        String txSummary = "DMR: Slot" + String(currentTx[i].slotNo) + " Seq=" + String(currentTx[i].startSeq) + "-" + String(currentTx[i].lastSeq) + 
                          " " + String(currentTx[i].srcId) + "->" + (currentTx[i].isGroup ? "TG" : "") + String(currentTx[i].dstId) +
                          " [END]";
        logSerial(txSummary);
      }
      currentTx[i].active = false;
    }
  }

  // Handle network communication
  if (wifiConnected) {
    handleNetwork();

    // Check for DMR login timeout and retry if needed
    if (mode_dmr_enabled && !dmrLoggedIn && wifiConnected) {
      if (dmrState == DMR_STATE::WAITING_LOGIN ||
          dmrState == DMR_STATE::WAITING_AUTH ||
          dmrState == DMR_STATE::WAITING_CONFIG) {

        if (currentMillis - lastLoginAttempt >= DMR_LOGIN_TIMEOUT) {
          if (loginAttempts < DMR_LOGIN_MAX_RETRIES) {
            loginAttempts++;
            logSerial("DMR login timeout - retrying (" + String(loginAttempts) + "/" + String(DMR_LOGIN_MAX_RETRIES) + ")");
            dmrLoginStatus = "Retrying... (" + String(loginAttempts) + "/" + String(DMR_LOGIN_MAX_RETRIES) + ")";
            connectToDMRNetwork();
            lastLoginAttempt = currentMillis;
#if ENABLE_OLED
            updateOLEDStatus(); // Update display to show retry status
#endif
          } else {
            logSerial("DMR login failed after " + String(DMR_LOGIN_MAX_RETRIES) + " attempts");
            dmrLoginStatus = "Login Failed";
            dmrState = DMR_STATE::DISCONNECTED;
#if ENABLE_OLED
            updateOLEDStatus(); // Update display to show failure
#endif
          }
        }
      }
    }

    // Send keepalive packets only if DMR mode is enabled and connected
    if (mode_dmr_enabled && dmrLoggedIn) {
      if (currentMillis - lastKeepalive >= NETWORK_KEEPALIVE_INTERVAL) {
        sendDMRKeepalive();
        lastKeepalive = currentMillis;
      }
    }
  }

  // Small delay to prevent watchdog issues
  delay(1);
}

void setupWiFi() {
  logSerial("Connecting to WiFi: " + String(ssid));

  setLEDMode(LED_MODE::FAST_BLINK);  // Fast blink while connecting

  // Prevent ESP32 WiFi library from auto-connecting with stored credentials
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);        // Disable WiFi credential storage in flash (must be before disconnect/begin)
  WiFi.setAutoReconnect(false);  // Disable auto-reconnect to prevent using old credentials
  WiFi.disconnect(true, true);   // Disconnect and erase any stored credentials from WiFi library
  delay(100);

  WiFi.setHostname(device_hostname.c_str());                        // Set WiFi hostname (must be before WiFi.begin)
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);  // Clear any previous config
  WiFi.setHostname(device_hostname.c_str());                        // Set hostname again after config
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    updateStatusLED();  // Update LED during connection attempts
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    setLEDMode(LED_MODE::STEADY);  // Steady on when connected
#if ENABLE_RGB_LED
    rgbLed.setStatus(RGBLedStatus::NETWORK_CONNECTED);
#endif
    logSerial("\nWiFi Connected!");
    logSerial("IP Address: " + WiFi.localIP().toString());

    // Start UDP
    udp.begin(LOCAL_PORT);
  } else {
    logSerial("\nWiFi Connection Failed!");

    // Try all configured alternate WiFi networks
    for (int i = 0; i < 5; i++) {
      if (wifiNetworks[i].ssid.length() > 0) {
        logSerial("Trying WiFi [" + wifiNetworks[i].label + "]: " + wifiNetworks[i].ssid);
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);  // Clear any previous config
        WiFi.setHostname(device_hostname.c_str());                        // Set WiFi hostname
        WiFi.begin(wifiNetworks[i].ssid.c_str(), wifiNetworks[i].password.c_str());

        attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 30) {
          updateStatusLED();  // Update LED during connection attempts
          delay(500);
          Serial.print(".");
          attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
          wifiConnected = true;
          setLEDMode(LED_MODE::STEADY);  // Steady on when connected
#if ENABLE_RGB_LED
          rgbLed.setStatus(RGBLedStatus::NETWORK_CONNECTED);
#endif
          logSerial("\nWiFi Connected [" + wifiNetworks[i].label + "]!");
          logSerial("IP Address: " + WiFi.localIP().toString());
          udp.begin(LOCAL_PORT);
          return;
        }
      }
    }

    // If all fails, start Access Point
    logSerial("Starting Access Point mode...");
    setupAccessPoint();
  }
}

void setupAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  setLEDMode(LED_MODE::SLOW_BLINK);  // Slow blink in AP mode
#if ENABLE_RGB_LED
  rgbLed.setStatus(RGBLedStatus::AP_MODE);
#endif

  IPAddress IP = WiFi.softAPIP();
  logSerial("AP IP address: " + IP.toString());
  logSerial("AP SSID: " + String(ap_ssid));
  logSerial("AP Password: " + String(ap_password));

  apMode = true;
}

#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
void WiFiEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      logSerial("ETH Started");
      ETH.setHostname(device_hostname.c_str());
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      logSerial("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      logSerial("ETH MAC: " + ETH.macAddress());
      logSerial("ETH IPv4: " + ETH.localIP().toString());
      if (ETH.fullDuplex()) {
        logSerial("ETH Mode: FULL_DUPLEX");
      }
      logSerial("ETH Speed: " + String(ETH.linkSpeed()) + "Mbps");
      logSerial("ETH Gateway: " + ETH.gatewayIP().toString());
      eth_connected = true;
      wifiConnected = true;
      setLEDMode(LED_MODE::STEADY);
#if ENABLE_RGB_LED
      rgbLed.setStatus(RGBLedStatus::NETWORK_CONNECTED);
#endif

      // Start UDP for DMR network
      udp.begin(LOCAL_PORT);
      logSerial("UDP started on port " + String(LOCAL_PORT));
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      logSerial("ETH Disconnected");
      eth_connected = false;
      // Don't touch wifiConnected - WiFi may still be active
#if ENABLE_RGB_LED
      // Only set disconnected status if WiFi is also not connected
      if (!wifiConnected) {
        rgbLed.setStatus(RGBLedStatus::DISCONNECTED);
      }
#endif
      break;
    case ARDUINO_EVENT_ETH_STOP:
      logSerial("ETH Stopped");
      eth_connected = false;
      // Don't touch wifiConnected - WiFi may still be active
      break;
    default:
      break;
  }
}
#endif  // LILYGO_T_ETH_ELITE_ESP32S3_MMDVM

#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
void setupEthernet() {
  logSerial("Initializing Ethernet (LILYGO T-ETH-ELITE)...");

  WiFi.onEvent(WiFiEvent);

  setLEDMode(LED_MODE::FAST_BLINK);

#ifdef ETH_POWER_PIN
  pinMode(ETH_POWER_PIN, OUTPUT);
  digitalWrite(ETH_POWER_PIN, HIGH);
#endif

  // ESP32-S3 uses W5500 Ethernet chip
  if (!ETH.begin(ETH_PHY_W5500, 1, ETH_CS_PIN, ETH_INT_PIN, ETH_RST_PIN,
                 SPI3_HOST,
                 ETH_SCLK_PIN, ETH_MISO_PIN, ETH_MOSI_PIN)) {
    logSerial("ETH start Failed!");
    logSerial("ERROR: Could not initialize Ethernet hardware!");
    logSerial("Please check your wiring and board configuration.");
  } else {
    logSerial("ETH initialization started");
    logSerial("Ethernet will connect in background...");
    // Give it a moment to start connecting
    delay(2000);
  }
}
#endif  // LILYGO_T_ETH_ELITE_ESP32S3_MMDVM

void setupMMDVM() {
  logSerial("Initializing MMDVM...");
  
  // Wake up MMDVM by starting continuous serial on GPIO 13
  // The modem needs ongoing UART activity on GPIO 13 to wake up and stay active
  logSerial("Starting MMDVM wakeup serial on GPIO " + String(MMDVM_WAKEUP_PIN) + "...");
  
  MMDVMWakeup.begin(115200, SERIAL_8N1, 1, MMDVM_WAKEUP_PIN);  // RX=1, TX=13
  mmdvmWakeupActive = true;
  delay(100);
  
  // Send initial wakeup burst and try to get version from wakeup responses
  uint8_t wakeCmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};
  bool versionFromWakeup = false;
  uint8_t wakeRxBuffer[256];
  int wakeRxPtr = 0;

  for (int i = 0; i < 20 && !versionFromWakeup; i++) {
    MMDVMWakeup.write(wakeCmd, 3);
    MMDVMWakeup.flush();
    delay(100); // Longer delay to allow response

    // Check for response on wakeup serial
    unsigned long wakeTimeout = millis() + 50;
    while (millis() < wakeTimeout && MMDVMWakeup.available()) {
      uint8_t byte = MMDVMWakeup.read();

      if (wakeRxPtr == 0 && byte != MMDVM_FRAME_START) {
        continue;
      }

      wakeRxBuffer[wakeRxPtr++] = byte;

      if (wakeRxPtr >= 2) {
        uint8_t frameLength = wakeRxBuffer[1];

        if (wakeRxPtr >= frameLength) {
          if (wakeRxPtr >= 3 && wakeRxBuffer[2] == CMD_GET_VERSION && wakeRxPtr > 4) {
            modemFirmwareVersion = ""; // Clear existing version
            for (int j = 4; j < wakeRxPtr && wakeRxBuffer[j] != 0x00; j++) {
              if (wakeRxBuffer[j] >= 32 && wakeRxBuffer[j] < 127) {
                modemFirmwareVersion += (char)wakeRxBuffer[j];
              }
            }
            logSerial("Modem Firmware (from wakeup): " + modemFirmwareVersion);
            versionFromWakeup = true;
          }
          wakeRxPtr = 0;
        }
      }

      if (wakeRxPtr >= sizeof(wakeRxBuffer)) {
        wakeRxPtr = 0;
      }
    }
  }

  logSerial("MMDVM wakeup active (SVC LED should be blinking)");

  // Clear any pending data from wakeup responses
  while (MMDVM_SERIAL.available()) {
    MMDVM_SERIAL.read();
  }

  delay(1000); // Give modem more time to stabilize

  // If we didn't get version from wakeup, try main UART
  if (!versionFromWakeup) {
    logSerial("Requesting modem firmware version on main UART...");
    sendMMDVMCommand(CMD_GET_VERSION, NULL, 0);
    MMDVM_SERIAL.flush();

    // Wait for version response
    unsigned long versionTimeout = millis() + 3000; // 3 second timeout
    bool versionReceived = false;
    uint8_t tempRxBuffer[256];
    int tempRxPtr = 0;

    while (millis() < versionTimeout && !versionReceived) {
      if (MMDVM_SERIAL.available()) {
        uint8_t byte = MMDVM_SERIAL.read();

        if (tempRxPtr == 0 && byte != MMDVM_FRAME_START) {
          continue;
        }

        tempRxBuffer[tempRxPtr++] = byte;

        if (tempRxPtr >= 2) {
          uint8_t frameLength = tempRxBuffer[1];

          if (tempRxPtr >= frameLength) {
            // Check if this is a version response
            if (tempRxPtr >= 3 && tempRxBuffer[2] == CMD_GET_VERSION) {
              modemFirmwareVersion = ""; // Clear existing version
              for (int i = 4; i < tempRxPtr && tempRxBuffer[i] != 0x00; i++) {
                if (tempRxBuffer[i] >= 32 && tempRxBuffer[i] < 127) {
                  modemFirmwareVersion += (char)tempRxBuffer[i];
                }
              }
              logSerial("Modem Firmware: " + modemFirmwareVersion);
              versionReceived = true;
            }
            tempRxPtr = 0;
          }
        }

        if (tempRxPtr >= sizeof(tempRxBuffer)) {
          tempRxPtr = 0;
        }
      }
      delay(10);
    }

    if (!versionReceived) {
      logSerial("Warning: No version response from modem");
    }
  }

  // IMPORTANT: Set frequency BEFORE config (per MMDVMHost sequence)
  // This ensures the radio is tuned before mode-specific settings are applied
  uint8_t rfPower = 100;  // RF power level (0-255, typically 100)
  logSerial("Setting modem frequency...");
  sendFrequency(dmr_rx_freq, dmr_tx_freq, rfPower);
  delay(100);

  // Set configuration (must be at least 23 bytes per firmware)
  uint8_t config[23];
  memset(config, 0, sizeof(config));

  // Byte 0: Flags (bit 7=simplex, bit 4=debug, bit 3=YSF low dev)
  config[0] = 0x80;  // Simplex mode

  // Byte 1: Mode enables (bitfield)
  uint8_t modeEnables = 0x00;
  if (mode_dstar_enabled) modeEnables |= 0x01;
  if (mode_dmr_enabled) modeEnables |= 0x02;
  if (mode_ysf_enabled) modeEnables |= 0x04;
  if (mode_p25_enabled) modeEnables |= 0x08;
  if (mode_nxdn_enabled) modeEnables |= 0x10;
  if (mode_pocsag_enabled) modeEnables |= 0x20;
  config[1] = modeEnables;

  // Byte 2: TX delay (0-50)
  config[2] = MMDVM_TX_DELAY;

  // Byte 3: Modem state (0x00=IDLE, 0x01=DSTAR, 0x02=DMR, etc.)
  uint8_t modemState = 0x00;  // Start in IDLE, will set mode separately
  if (mode_dmr_enabled) modemState = 0x02;  // DMR
  else if (mode_dstar_enabled) modemState = 0x01;  // D-Star
  else if (mode_ysf_enabled) modemState = 0x03;  // YSF
  else if (mode_p25_enabled) modemState = 0x04;  // P25
  else if (mode_nxdn_enabled) modemState = 0x05;  // NXDN
  else if (mode_pocsag_enabled) modemState = 0x07;  // POCSAG
  config[3] = modemState;

  // Byte 4: RX level
  config[4] = MMDVM_RX_LEVEL;

  // Byte 5: CW ID TX level (shifted by 2)
  config[5] = MMDVM_TX_LEVEL;

  // Byte 6: DMR Color Code (0-15) - CRITICAL for DMR operation
  config[6] = dmr_color_code;

  // Byte 7: DMR delay (duplex only, 0 for simplex)
  config[7] = 0x00;

  // Bytes 9-21: TX levels for each mode
  config[9] = MMDVM_TX_LEVEL;   // D-Star TX level
  config[10] = MMDVM_TX_LEVEL;  // DMR TX level
  config[11] = MMDVM_TX_LEVEL;  // YSF TX level
  config[12] = MMDVM_TX_LEVEL;  // P25 TX level
  config[15] = MMDVM_TX_LEVEL;  // NXDN TX level
  config[17] = MMDVM_TX_LEVEL;  // POCSAG TX level
  config[21] = MMDVM_TX_LEVEL;  // M17 TX level

  String modeStr = "IDLE";
  if (modemState == 0x02) modeStr = "DMR";
  else if (modemState == 0x01) modeStr = "D-Star";
  else if (modemState == 0x03) modeStr = "YSF";
  else if (modemState == 0x04) modeStr = "P25";
  else if (modemState == 0x05) modeStr = "NXDN";
  else if (modemState == 0x07) modeStr = "POCSAG";

  logSerial("Mode enables - DMR: " + String(mode_dmr_enabled ? "ON" : "OFF") +
            " D-Star: " + String(mode_dstar_enabled ? "ON" : "OFF"));
  logSerial("Config byte[1] (mode enables): 0x" + String(modeEnables, HEX) +
            " byte[3] (state): 0x" + String(modemState, HEX));
  logSerial("Setting modem configuration (Mode: " + modeStr + ", Color Code: " + String(dmr_color_code) + ")...");
  sendMMDVMCommand(CMD_SET_CONFIG, config, 23);
  delay(200);  // Give modem time to configure and set mode

  // Note: Mode is set within CMD_SET_CONFIG (byte 3), no need for separate CMD_SET_MODE
  // The firmware will automatically configure the radio and set the DMR LED

  // Only mark modem as ready if we successfully received firmware version
  if (modemFirmwareVersion != "Unknown" && modemFirmwareVersion.length() > 0) {
    logSerial("MMDVM Initialized - Modem Ready");
    mmdvmReady = true;
  } else {
    logSerial("MMDVM Initialized - WARNING: No modem firmware version detected!");
    mmdvmReady = false;
  }
}

void sendMMDVMCommand(uint8_t cmd, uint8_t* data, uint16_t length) {
  uint8_t buffer[512];
  buffer[0] = MMDVM_FRAME_START;
  buffer[1] = length + 3;  // Length includes START, LEN, and CMD
  buffer[2] = cmd;

  if (data != NULL && length > 0) {
    memcpy(&buffer[3], data, length);
  }

  MMDVM_SERIAL.write(buffer, length + 3);
}

void sendFrequency(uint32_t rxFreq, uint32_t txFreq, uint8_t rfPower) {
  // CMD_SET_FREQ payload format per MMDVM_HS firmware (SerialPort.cpp line 548):
  // data[0]:    Mode/flags byte (unused in simplex mode, set to 0)
  // data[1-4]:  RX frequency in Hz (little-endian)
  // data[5-8]:  TX frequency in Hz (little-endian)
  // data[9]:    RF power level (0-255)
  // data[10-13]: POCSAG TX frequency in Hz (little-endian)
  uint8_t freqData[14];

  // Byte 0: Mode/flags (unused, set to 0)
  freqData[0] = 0x00;

  // RX Frequency (little-endian) - starts at byte 1!
  freqData[1] = (rxFreq >> 0) & 0xFF;
  freqData[2] = (rxFreq >> 8) & 0xFF;
  freqData[3] = (rxFreq >> 16) & 0xFF;
  freqData[4] = (rxFreq >> 24) & 0xFF;

  // TX Frequency (little-endian) - starts at byte 5
  freqData[5] = (txFreq >> 0) & 0xFF;
  freqData[6] = (txFreq >> 8) & 0xFF;
  freqData[7] = (txFreq >> 16) & 0xFF;
  freqData[8] = (txFreq >> 24) & 0xFF;

  // RF Power level - byte 9
  freqData[9] = rfPower;

  // POCSAG TX frequency (little-endian) - bytes 10-13, use same as TX freq
  freqData[10] = (txFreq >> 0) & 0xFF;
  freqData[11] = (txFreq >> 8) & 0xFF;
  freqData[12] = (txFreq >> 16) & 0xFF;
  freqData[13] = (txFreq >> 24) & 0xFF;

  sendMMDVMCommand(CMD_SET_FREQ, freqData, 14);

  logSerial("Frequency set - RX: " + String(rxFreq) + " Hz, TX: " + String(txFreq) + " Hz, Power: " + String(rfPower));
}

void writeDMRStart(bool tx) {
  // Send DMR START command to tell modem to enter/exit TX mode
  // This is critical - without it, the modem won't transmit!
  uint8_t buffer[4];
  buffer[0] = MMDVM_FRAME_START;
  buffer[1] = 4;  // Length
  buffer[2] = CMD_DMR_START;
  buffer[3] = tx ? 0x01 : 0x00;  // 0x01 = start TX, 0x00 = stop TX
  
  MMDVM_SERIAL.write(buffer, 4);
  MMDVM_SERIAL.flush();
  
  logSerial(tx ? "DMR TX START" : "DMR TX STOP");
}

void handleMMDVMSerial() {
  while (MMDVM_SERIAL.available()) {
    uint8_t byte = MMDVM_SERIAL.read();

    if (rxBufferPtr == 0 && byte != MMDVM_FRAME_START) {
      continue;  // Wait for frame start
    }

    rxBuffer[rxBufferPtr++] = byte;

    // Check if we have enough bytes to read length
    if (rxBufferPtr >= 2) {
      uint8_t frameLength = rxBuffer[1];

      // Check if we have a complete frame
      if (rxBufferPtr >= frameLength) {
        processMMDVMFrame();
        rxBufferPtr = 0;
      }
    }

    // Prevent buffer overflow
    if (rxBufferPtr >= sizeof(rxBuffer)) {
      rxBufferPtr = 0;
    }
  }
}

void processMMDVMFrame() {
  if (rxBufferPtr < 3) return;

  uint8_t cmd = rxBuffer[2];

  switch (cmd) {
    case CMD_GET_VERSION:
      {
        String version = "MMDVM Version: ";
        for (int i = 3; i < rxBufferPtr; i++) {
          version += (char)rxBuffer[i];
        }
        logSerial(version);
      }
      break;

    case CMD_GET_STATUS:
      {
        String status = "MMDVM Status - Mode: " + String(rxBuffer[3]) + " TX: " + String(rxBuffer[4]) + " Overflow: " + String(rxBuffer[7]);
        logSerial(status);
      }
      break;

    case CMD_ACK:
      // Modem acknowledged a command
      logSerialVerbose("MMDVM ACK received");
      break;

    case CMD_NAK:
      // Modem rejected a command
      // NAK format: [START] [LEN] [NAK=0x7F] [CMD_REJECTED] [ERROR_CODE]
      if (rxBufferPtr >= 5) {
        uint8_t rejectedCmd = rxBuffer[3];
        uint8_t reason = rxBuffer[4];
        String cmdStr;
        switch (rejectedCmd) {
          case CMD_GET_VERSION: cmdStr = "GET_VERSION"; break;
          case CMD_GET_STATUS: cmdStr = "GET_STATUS"; break;
          case CMD_SET_CONFIG: cmdStr = "SET_CONFIG"; break;
          case CMD_SET_MODE: cmdStr = "SET_MODE"; break;
          case CMD_SET_FREQ: cmdStr = "SET_FREQ"; break;
          case CMD_DMR_DATA1: cmdStr = "DMR_DATA1"; break;
          case CMD_DMR_DATA2: cmdStr = "DMR_DATA2"; break;
          default: cmdStr = "0x" + String(rejectedCmd, HEX); break;
        }
        String reasonStr;
        switch (reason) {
          case 1: reasonStr = "Invalid Command"; break;
          case 2: reasonStr = "Wrong Mode"; break;
          case 3: reasonStr = "Command Too Long"; break;
          case 4: reasonStr = "Data Incorrect"; break;
          case 5: reasonStr = "Not Enough Buffer Space"; break;
          default: reasonStr = "Unknown (" + String(reason) + ")"; break;
        }
        logSerial("MMDVM NAK - Command: " + cmdStr + ", Reason: " + reasonStr);
      } else if (rxBufferPtr >= 4) {
        uint8_t reason = rxBuffer[3];
        String reasonStr;
        switch (reason) {
          case 1: reasonStr = "Invalid Command"; break;
          case 2: reasonStr = "Wrong Mode"; break;
          case 3: reasonStr = "Command Too Long"; break;
          case 4: reasonStr = "Data Incorrect"; break;
          case 5: reasonStr = "Not Enough Buffer Space"; break;
          default: reasonStr = "Unknown (" + String(reason) + ")"; break;
        }
        logSerial("MMDVM NAK - Reason: " + reasonStr);
      } else {
        logSerial("MMDVM NAK received");
      }
      break;

    case CMD_DMR_DATA1:
    case CMD_DMR_DATA2:
      // DMR data received from MMDVM - forward to network (TRANSMITTING)
      if (wifiConnected) {
        // Extract DMR frame and send to network
        uint16_t dataLen = rxBufferPtr - 3;
        udp.beginPacket(dmr_server.c_str(), dmr_port);
        udp.write(&rxBuffer[3], dataLen);
        udp.endPacket();

        logSerial("DMR data forwarded to network");
        digitalWrite(COS_LED_PIN, HIGH);
#if ENABLE_RGB_LED
        rgbLed.setStatus(RGBLedStatus::TRANSMITTING);
#endif
        delay(50);
        digitalWrite(COS_LED_PIN, LOW);
#if ENABLE_RGB_LED
        rgbLed.setStatus(RGBLedStatus::IDLE_CONNECTED);
#endif
      }
      break;

    default:
      logSerial("Unknown MMDVM command: 0x" + String(cmd, HEX));
      break;
  }
}

void handleNetwork() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    uint8_t packet[512];
    int len = udp.read(packet, sizeof(packet));

    if (len > 0) {
      // Check if this is a keepalive packet first (for conditional logging)
      bool isKeepalive = (memcmp(packet, "MSTPONG", 7) == 0 && len >= 7);
      bool isDMRData = (memcmp(packet, "DMRD", 4) == 0 && len >= 55);

      // Only log hex dump for non-DMR data packets (DMR gets decoded below)
      if (!isDMRData) {
        String hexDump = "RX [" + String(len) + "]: ";
        for (int i = 0; i < min(len, 16); i++) {
          if (packet[i] < 0x10) hexDump += "0";
          hexDump += String(packet[i], HEX);
          hexDump += " ";
        }

        // Use verbose logging for keepalive packets (always USB, conditionally web)
        if (isKeepalive) {
          logSerialVerbose(hexDump);
        } else {
          logSerial(hexDump);
        }
      }

      // Check for BrandMeister responses (binary comparison)
      if (len >= 4) {
        // Check for negative acknowledgment (MSTNAK)
        if (memcmp(packet, "MSTNAK", 6) == 0 && len >= 6) {
          dmrLoggedIn = false;
          dmrLoginStatus = "Login Failed";

          // Log which stage failed
          String stageMsg = "BrandMeister NAK at stage: ";
          switch (dmrState) {
            case DMR_STATE::WAITING_LOGIN: stageMsg += "LOGIN"; break;
            case DMR_STATE::WAITING_AUTH: stageMsg += "AUTH"; break;
            case DMR_STATE::WAITING_CONFIG: stageMsg += "CONFIG"; break;
            default: stageMsg += "UNKNOWN";
          }
          logSerial(stageMsg);
          logSerial("STOPPING - Please check configuration and reboot");

          // Stop trying to prevent ban
          dmrState = DMR_STATE::DISCONNECTED;
        }
        // Check for login acknowledgment (RPTACK)
        else if (memcmp(packet, "RPTACK", 6) == 0 && len >= 10) {
          // RPTACK response includes salt for password authentication
          memcpy(dmrSalt, packet + 6, 4);

          // Debug: Show salt
          String saltHex = "Salt: ";
          for (int i = 0; i < 4; i++) {
            if (dmrSalt[i] < 0x10) saltHex += "0";
            saltHex += String(dmrSalt[i], HEX);
          }
          logSerial(saltHex);

          switch (dmrState) {
            case DMR_STATE::WAITING_LOGIN:
              logSerial("Login ACK received (state: WAITING_LOGIN), sending auth...");
              dmrState = DMR_STATE::WAITING_AUTH;
              sendDMRAuth();
              break;
            case DMR_STATE::WAITING_AUTH:
              logSerial("Auth ACK received (state: WAITING_AUTH), sending config...");
              dmrState = DMR_STATE::WAITING_CONFIG;
              sendDMRConfig();
              break;
            case DMR_STATE::WAITING_CONFIG:
              logSerial("Config ACK - CONNECTED!");
              dmrLoggedIn = true;
              dmrLoginStatus = "Connected";
              dmrState = DMR_STATE::CONNECTED;
              loginAttempts = 0; // Reset retry counter on successful login
              logSerial("DMR Network fully connected and operational!");

              // Force immediate OLED update to show connected status
#if ENABLE_OLED
              updateOLEDStatus();
              lastOLEDUpdate = millis(); // Reset timer
#endif
              break;
            case DMR_STATE::DISCONNECTED:
              // Don't retry after NAK to prevent bans
              logSerial("RPTACK received but in DISCONNECTED state - ignoring");
              break;
            default:
              logSerial("RPTACK received in unexpected state: " + String((int)dmrState));
              break;
          }
        }
        // Check for ping response (MSTPONG)
        else if (memcmp(packet, "MSTPONG", 7) == 0 && len >= 7) {
          logSerialVerbose("Keepalive ACK");
        }
        // DMR data packet
        else if (memcmp(packet, "DMRD", 4) == 0 && len >= 55) {
          // Parse DMR packet structure
          uint8_t seqNo = packet[4];
          uint32_t srcId = (packet[5] << 16) | (packet[6] << 8) | packet[7];
          uint32_t dstId = (packet[8] << 16) | (packet[9] << 8) | packet[10];
          uint32_t rptId = (packet[11] << 24) | (packet[12] << 16) | (packet[13] << 8) | packet[14];
          
          uint8_t controlByte = packet[15];
          uint8_t slotNo = (controlByte & 0x80) ? 2 : 1;
          bool isGroup = (controlByte & 0x40) == 0;  // 0=Group, 1=Private
          bool dataSync = (controlByte & 0x20) != 0;
          bool voiceSync = (controlByte & 0x10) != 0;
          uint8_t dataType = controlByte & 0x0F;
          
          uint8_t ber = packet[53];
          uint8_t rssi = packet[54];
          
          // Data type names
          const char* dataTypeStr = "UNKNOWN";
          switch (dataType) {
            case 0x00: dataTypeStr = "PI_HEADER"; break;
            case 0x01: dataTypeStr = "VOICE_LC_HDR"; break;
            case 0x02: dataTypeStr = "TERM_LC"; break;
            case 0x03: dataTypeStr = "CSBK"; break;
            case 0x06: dataTypeStr = "DATA_HDR"; break;
            case 0x07: dataTypeStr = "RATE_1/2_DATA"; break;
            case 0x08: dataTypeStr = "RATE_3/4_DATA"; break;
            case 0x09: dataTypeStr = "IDLE"; break;
            case 0x0A: dataTypeStr = "RATE_1_DATA"; break;
          }
          
          // If it's a voice frame (no specific data type flag), show as VOICE
          if (!dataSync && voiceSync) {
            dataTypeStr = "VOICE";
          } else if (!dataSync && !voiceSync) {
            dataTypeStr = "VOICE_BURST";
          }
          
          // Check if this is a TERM_LC (transmission end marker)
          bool isTermLC = (dataType == 0x02); // TERM_LC
          
          // Build readable log message with consolidated transmission tracking
          int txIndex = slotNo - 1;
          DMRTransmission &tx = currentTx[txIndex];
          
          // Check if this is a new transmission (different source/dest or was inactive)
          bool isNewTransmission = !tx.active || tx.srcId != srcId || tx.dstId != dstId;
          
          // TERM_LC within the same transmission is just a superframe marker, not the actual end
          // Only end transmission if we haven't seen frames for a while or source/dest changed
          if (isNewTransmission) {
            // Log the previous transmission summary if it was active
            if (tx.active && tx.lastSeq > tx.startSeq) {
              String txSummary = "DMR: Slot" + String(tx.slotNo) + " Seq=" + String(tx.startSeq) + "-" + String(tx.lastSeq) + 
                                " " + String(tx.srcId) + "->" + (tx.isGroup ? "TG" : "") + String(tx.dstId) +
                                " [END]";
              logSerial(txSummary);
            }
            
            // Start new transmission tracking
            tx.srcId = srcId;
            tx.dstId = dstId;
            tx.slotNo = slotNo;
            tx.isGroup = isGroup;
            tx.startSeq = seqNo;
            tx.lastSeq = seqNo;
            tx.active = true;
            tx.frameType = String(dataTypeStr);
            
            // Log the start of transmission
            String dmrInfo = "DMR: Slot" + String(slotNo) + " Seq=" + String(seqNo) + 
                            " " + String(srcId) + "->" + (isGroup ? "TG" : "") + String(dstId) +
                            " [START] Type=" + String(dataTypeStr);
            if (ber > 0 || rssi > 0) {
              dmrInfo += " BER=" + String(ber) + " RSSI=" + String(rssi);
            }
            logSerial(dmrInfo);
          } else {
            // Continue existing transmission - just update sequence
            tx.lastSeq = seqNo;
            // Update frame type if it changed (VOICE -> VOICE_BURST -> TERM_LC are all part of same transmission)
            if (tx.frameType != String(dataTypeStr)) {
              tx.frameType = String(dataTypeStr);
            }
            // Don't log individual frames within a transmission
          }
          
          // Update DMR activity tracking
          int activityIndex = slotNo - 1;  // Slot 1 = index 0, Slot 2 = index 1
          
          // Check if we need to add previous transmission to history (DMR ID changed)
          if (dmrActivity[activityIndex].active && dmrActivity[activityIndex].srcId > 0 && dmrActivity[activityIndex].srcId != srcId) {
            // Previous transmission ended, add it to history
            uint32_t duration = (millis() - dmrActivity[activityIndex].startTime) / 1000;
            String location = "";
            if (dmrActivity[activityIndex].srcCity.length() > 0 || dmrActivity[activityIndex].srcCountry.length() > 0) {
              if (dmrActivity[activityIndex].srcCity.length() > 0) location += dmrActivity[activityIndex].srcCity;
              if (dmrActivity[activityIndex].srcCity.length() > 0 && dmrActivity[activityIndex].srcCountry.length() > 0) location += ", ";
              if (dmrActivity[activityIndex].srcCountry.length() > 0) location += dmrActivity[activityIndex].srcCountry;
            }
            addDMRHistory(dmrActivity[activityIndex].srcId, dmrActivity[activityIndex].srcCallsign, 
                         dmrActivity[activityIndex].srcName, location, dmrActivity[activityIndex].dstId, 
                         dmrActivity[activityIndex].isGroup, duration, 0, 0, dmrActivity[activityIndex].slotNo);
          }
          
          // Only set start time and lookup user info if this is a new transmission (not just another frame)
          if (!dmrActivity[activityIndex].active || 
              dmrActivity[activityIndex].srcId != srcId || 
              dmrActivity[activityIndex].dstId != dstId) {
            dmrActivity[activityIndex].startTime = millis();   // Actual transmission start time
            dmrActivity[activityIndex].lastUpdate = millis();  // Keep for timeout detection
            
            // Lookup detailed user information (async, will use cache if available)
            String userInfo = lookupUserInfo(srcId);
            // userInfo format: "callsign|name|city|country" or just "callsign" for basic lookup
            int pipe1 = userInfo.indexOf('|');
            if (pipe1 > 0) {
              dmrActivity[activityIndex].srcCallsign = userInfo.substring(0, pipe1);
              int pipe2 = userInfo.indexOf('|', pipe1 + 1);
              if (pipe2 > pipe1) {
                dmrActivity[activityIndex].srcName = userInfo.substring(pipe1 + 1, pipe2);
                int pipe3 = userInfo.indexOf('|', pipe2 + 1);
                if (pipe3 > pipe2) {
                  dmrActivity[activityIndex].srcCity = userInfo.substring(pipe2 + 1, pipe3);
                  dmrActivity[activityIndex].srcCountry = userInfo.substring(pipe3 + 1);
                } else {
                  dmrActivity[activityIndex].srcCity = userInfo.substring(pipe2 + 1);
                }
              } else {
                dmrActivity[activityIndex].srcName = userInfo.substring(pipe1 + 1);
              }
            } else {
              dmrActivity[activityIndex].srcCallsign = userInfo;
            }
            
            // Log with enhanced info if found
            if (dmrActivity[activityIndex].srcCallsign.length() > 0 && isNewTransmission) {
              String logMsg = "Station: " + dmrActivity[activityIndex].srcCallsign + " (" + String(srcId) + ")";
              if (dmrActivity[activityIndex].srcName.length() > 0) {
                logMsg += " - " + dmrActivity[activityIndex].srcName;
              }
              if (dmrActivity[activityIndex].srcCity.length() > 0) {
                logMsg += " from " + dmrActivity[activityIndex].srcCity;
                if (dmrActivity[activityIndex].srcCountry.length() > 0) {
                  logMsg += ", " + dmrActivity[activityIndex].srcCountry;
                }
              }
              logSerial(logMsg);
            }
          } else {
            // Update lastUpdate for timeout detection but keep startTime unchanged
            dmrActivity[activityIndex].lastUpdate = millis();
          }
          
          dmrActivity[activityIndex].srcId = srcId;
          dmrActivity[activityIndex].dstId = dstId;
          dmrActivity[activityIndex].slotNo = slotNo;
          dmrActivity[activityIndex].isGroup = isGroup;
          dmrActivity[activityIndex].frameType = String(dataTypeStr);
          dmrActivity[activityIndex].active = true;
          
          // Update current talkgroup for quick status
          if (isGroup) {
            currentTalkgroup = dstId;
          }
          
          // Mark that we've seen TERM_LC for this transmission (but don't add to history yet)
          // History will be added when the transmission times out and becomes inactive

          // Parse and forward to MMDVM (RECEIVING from network)
          if (mmdvmReady) {
            // Extract DMR frame from network packet
            // BrandMeister DMRD packet structure (55 bytes):
            //   Bytes 0-3: "DMRD" magic
            //   Byte 4: Sequence number
            //   Bytes 5-7: Source ID
            //   Bytes 8-10: Destination ID
            //   Bytes 11-14: Repeater ID
            //   Byte 15: Control flags (slot, group/private, sync flags, data type)
            //   Bytes 16-19: Stream ID
            //   Bytes 20-52: DMR frame data (33 bytes) <-- THIS IS WHAT WE NEED
            //   Byte 53: BER
            //   Byte 54: RSSI

            // MMDVM modem expects (34 bytes):
            //   Byte 0: Control byte (usually 0x00)
            //   Bytes 1-33: DMR frame data (33 bytes from network packet bytes 20-52)
            //
            // Note: MMDVMHost uses TAG_DATA/TAG_EOT internally but does NOT send it to the modem
            // The TAG is stripped before transmission (see Modem.cpp line 1253)

            uint8_t dmrModemData[34];
            dmrModemData[0] = 0x00;  // Control byte
            memcpy(&dmrModemData[1], &packet[20], 33);  // Copy 33-byte DMR frame

            // Debug logging
            String debugMsg = "TX->Modem: Slot" + String(slotNo) + " Len=" + String(34) +
                             " Ctrl=" + String(dmrModemData[0], HEX) +
                             " Frame[0-3]=" + String(dmrModemData[1], HEX) + " " +
                             String(dmrModemData[2], HEX) + " " +
                             String(dmrModemData[3], HEX) + " " +
                             String(dmrModemData[4], HEX);
            logSerial(debugMsg);

            // Send DMR START command (tells modem to enter TX mode)
            writeDMRStart(true);
            
            uint8_t cmd = (slotNo == 1) ? CMD_DMR_DATA1 : CMD_DMR_DATA2;
            sendMMDVMCommand(cmd, dmrModemData, 34);
#if ENABLE_RGB_LED
            rgbLed.setStatus(RGBLedStatus::RECEIVING);
            delay(50);
            rgbLed.setStatus(RGBLedStatus::IDLE_CONNECTED);
#endif
          }
        }
      }
    }
  }
}

void connectToDMRNetwork() {
  dmrLoginStatus = "Connecting...";
  dmrLoggedIn = false;
  dmrState = DMR_STATE::WAITING_LOGIN;
  lastLoginAttempt = millis(); // Start timeout timer

  logSerial("Connecting to DMR Network...");
  logSerial("Server: " + dmr_server + ":" + String(dmr_port));
  logSerial("Callsign: " + dmr_callsign + " ID: " + String(dmr_id));
  if (dmr_essid > 0) {
    logSerial("ESSID: " + String(dmr_essid));
  }

  // Create proper BrandMeister login packet
  // Format: "RPTL" (4 bytes) + DMR_ID (4 bytes binary, big-endian)
  uint8_t loginPacket[8];
  uint32_t id_to_send = dmr_id;

  // If ESSID is set, append it to the ID (e.g., 204115201 for ID 2041152 with ESSID 01)
  if (dmr_essid > 0) {
    id_to_send = dmr_id * 100 + dmr_essid;
  }

  // Packet format: "RPTL" + 4-byte binary DMR ID (big-endian)
  memcpy(loginPacket, "RPTL", 4);
  loginPacket[4] = (id_to_send >> 24) & 0xFF;  // Most significant byte
  loginPacket[5] = (id_to_send >> 16) & 0xFF;
  loginPacket[6] = (id_to_send >> 8) & 0xFF;
  loginPacket[7] = id_to_send & 0xFF;  // Least significant byte

  udp.beginPacket(dmr_server.c_str(), dmr_port);
  udp.write(loginPacket, 8);
  udp.endPacket();

  logSerial("Login packet sent, ID: " + String(id_to_send));
}

void sendDMRAuth() {
// Send RPTK (authorization) packet with SHA256(salt + password)
// Format: "RPTK" + DMR_ID (4 bytes binary) + SHA256 hash (32 bytes binary)
// SHA256 input: salt (4 binary bytes) + password (ASCII string)

// Debug: Show password being used (with more detail)
#if DEBUG_PASSWORD
  String passDebug = "Using password: length=" + String(dmr_password.length());
  if (dmr_password.length() > 0) {
    passDebug += ", last4=" + dmr_password.substring(max(0, (int)dmr_password.length() - 4));
  } else {
    passDebug += " [EMPTY!]";
  }
  logSerial(passDebug);
#endif

  // Calculate SHA256 hash of (salt + password)
  size_t passLen = dmr_password.length();
  size_t inputLen = 4 + passLen;
  uint8_t* input = new uint8_t[inputLen];

  // Salt is 4 binary bytes, not hex string
  memcpy(input, dmrSalt, 4);
  memcpy(input + 4, dmr_password.c_str(), passLen);

  // Calculate SHA256 hash (binary output, not hex)
  uint8_t hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, input, inputLen);
  mbedtls_md_finish(&ctx, hash);
  mbedtls_md_free(&ctx);

  delete[] input;

  // Build RPTK packet: "RPTK" (4 bytes) + ID (4 bytes binary) + hash (32 bytes binary)
  uint8_t authPacket[40];
  uint32_t id_to_send = dmr_id;
  if (dmr_essid > 0) {
    id_to_send = dmr_id * 100 + dmr_essid;
  }

  memcpy(authPacket, "RPTK", 4);
  authPacket[4] = (id_to_send >> 24) & 0xFF;  // ID as 4 binary bytes (big-endian)
  authPacket[5] = (id_to_send >> 16) & 0xFF;
  authPacket[6] = (id_to_send >> 8) & 0xFF;
  authPacket[7] = id_to_send & 0xFF;
  memcpy(authPacket + 8, hash, 32);  // SHA256 hash as 32 binary bytes

  udp.beginPacket(dmr_server.c_str(), dmr_port);
  udp.write(authPacket, 40);
  udp.endPacket();

  logSerial("Auth packet sent (40 bytes)");
}

void sendDMRConfig() {
  // Send RPTC (configuration) packet
  // Format: "RPTC" (4 bytes) + DMR_ID (4 bytes binary) + config string (294 bytes) = 302 bytes
  // Using larger buffer to prevent overflow during snprintf, then copy exact 294 bytes
  char configString[400];
  memset(configString, 0, sizeof(configString));

  uint32_t id_to_send = dmr_id;
  if (dmr_essid > 0) {
    id_to_send = dmr_id * 100 + dmr_essid;
  }

  // Config format (callsign + basic info) - 294 bytes
  // Format per MMDVMHost writeConfig():
  // %-8.8s%09u%09u%02u%02u%8.8s%9.9s%03d%-20.20s%-19.19s%c%-124.124s%-40.40s%-40.40s

  // Format latitude and longitude properly as floats with exact widths
  char latitude[20];
  char longitude[20];
  sprintf(latitude, "%08f", dmr_latitude);    // %08f = 8 chars total
  sprintf(longitude, "%09f", dmr_longitude);  // %09f = 9 chars total

  // Validate and limit values
  unsigned int power = dmr_power;
  if (power > 99) power = 99;
  int height = dmr_height;
  if (height > 999) height = 999;

  snprintf(configString, sizeof(configString),
           "%-8.8s%09u%09u%02u%02u%8.8s%9.9s%03d%-20.20s%-19.19s%c%-124.124s%-40.40s%-40.40s",
           dmr_callsign.c_str(),     // Callsign (8 chars, left-aligned)
           dmr_rx_freq,              // RX Frequency (9 digits)
           dmr_tx_freq,              // TX Frequency (9 digits)
           power,                    // Power (2 digits)
           dmr_color_code,           // Color Code (2 digits)
           latitude,                 // Latitude (8 chars)
           longitude,                // Longitude (9 chars)
           height,                   // Height (3 digits)
           dmr_location.c_str(),     // Location (20 chars)
           dmr_description.c_str(),  // Description (19 chars)
           '4',                      // Slots (1 char: '4' = simplex)
           dmr_url.c_str(),          // URL (124 chars)
           firmwareVersion.c_str(),  // Version (40 chars) - from firmware variable
           "MMDVM_MMDVM_HS");        // Software (40 chars)

  // Debug: Log first 100 chars of config string
  String configDebug = "Config string preview: ";
  for (int i = 0; i < min(100, (int)strlen(configString)); i++) {
    if (configString[i] >= 32 && configString[i] <= 126) {
      configDebug += (char)configString[i];
    } else {
      configDebug += ".";
    }
  }
  logSerial(configDebug);
  logSerial("Config string length: " + String(strlen(configString)));

  // Build the packet with binary ID
  uint8_t configPacket[302];
  memset(configPacket, 0, sizeof(configPacket));

  memcpy(configPacket, "RPTC", 4);
  configPacket[4] = (id_to_send >> 24) & 0xFF;  // ID as 4 binary bytes (big-endian)
  configPacket[5] = (id_to_send >> 16) & 0xFF;
  configPacket[6] = (id_to_send >> 8) & 0xFF;
  configPacket[7] = id_to_send & 0xFF;
  memcpy(configPacket + 8, configString, 294);  // Copy exactly 294 bytes

  udp.beginPacket(dmr_server.c_str(), dmr_port);
  udp.write(configPacket, 302);
  udp.endPacket();

  logSerial("Config packet sent (302 bytes)");
}

void sendDMRKeepalive() {
  // Send keepalive/ping packet to DMR network
  // Format: "RPTPING" (7 bytes) + DMR_ID (4 bytes binary) = 11 bytes
  uint8_t keepalive[11];
  uint32_t id_to_send = dmr_id;

  if (dmr_essid > 0) {
    id_to_send = dmr_id * 100 + dmr_essid;
  }

  memcpy(keepalive, "RPTPING", 7);
  keepalive[7] = (id_to_send >> 24) & 0xFF;  // ID as 4 binary bytes (big-endian)
  keepalive[8] = (id_to_send >> 16) & 0xFF;
  keepalive[9] = (id_to_send >> 8) & 0xFF;
  keepalive[10] = id_to_send & 0xFF;

  udp.beginPacket(dmr_server.c_str(), dmr_port);
  udp.write(keepalive, 11);
  udp.endPacket();

  logSerialVerbose("Keepalive sent");
}

// ===== Serial Logging Functions =====
// Log to both USB serial and web buffer
void logSerial(String message) {
  Serial.println(message);

  // Store in circular buffer for web monitor
  serialLog[serialLogIndex] = message;
  serialLogIndex = (serialLogIndex + 1) % SERIAL_LOG_SIZE;
}

// Log with verbose flag - always to USB serial, conditionally to web buffer
void logSerialVerbose(String message) {
  // Always log to USB serial for debugging
  Serial.println(message);

  // Only store in web buffer if verbose logging is enabled
  if (verbose_logging) {
    serialLog[serialLogIndex] = message;
    serialLogIndex = (serialLogIndex + 1) % SERIAL_LOG_SIZE;
  }
}

#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
// ===== SD Card Helper Functions =====
bool writeSDFile(const char* path, const char* data) {
  if (!sdCardAvailable) return false;

  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    logSerial("Failed to open file for writing: " + String(path));
    return false;
  }

  if (file.print(data)) {
    file.close();
    return true;
  } else {
    logSerial("Write failed");
    file.close();
    return false;
  }
}

String readSDFile(const char* path) {
  if (!sdCardAvailable) return "";

  File file = SD.open(path);
  if (!file) {
    return "";
  }

  String content = "";
  while (file.available()) {
    content += (char)file.read();
  }
  file.close();
  return content;
}

bool appendSDFile(const char* path, const char* data) {
  if (!sdCardAvailable) return false;

  File file = SD.open(path, FILE_APPEND);
  if (!file) {
    logSerial("Failed to open file for appending: " + String(path));
    return false;
  }

  if (file.print(data)) {
    file.close();
    return true;
  } else {
    logSerial("Append failed");
    file.close();
    return false;
  }
}

bool deleteSDFile(const char* path) {
  if (!sdCardAvailable) return false;
  return SD.remove(path);
}

bool sdFileExists(const char* path) {
  if (!sdCardAvailable) return false;
  return SD.exists(path);
}
#endif

// ===== Configuration Load/Save =====
void loadConfig() {
  preferences.begin("mmdvm", false);

  // Load DMR settings if they exist
  if (preferences.isKey("dmr_callsign")) {
    dmr_callsign = preferences.getString("dmr_callsign", DMR_CALLSIGN);
    dmr_id = preferences.getUInt("dmr_id", DMR_ID);
    dmr_server = preferences.getString("dmr_server", DMR_SERVER);
    dmr_password = preferences.getString("dmr_password", DMR_PASSWORD);
    dmr_essid = preferences.getUChar("dmr_essid", 0);

    // Load additional config options
    dmr_rx_freq = preferences.getUInt("dmr_rx_freq", 434000000);
    dmr_tx_freq = preferences.getUInt("dmr_tx_freq", 434000000);
    dmr_power = preferences.getUChar("dmr_power", 10);
    dmr_color_code = preferences.getUChar("dmr_cc", 1);
    dmr_latitude = preferences.getFloat("dmr_lat", 0.0);
    dmr_longitude = preferences.getFloat("dmr_lon", 0.0);
    dmr_height = preferences.getInt("dmr_height", 0);
    dmr_location = preferences.getString("dmr_location", "ESP32 Hotspot");
    dmr_description = preferences.getString("dmr_desc", "ESP32-MMDVM");
    dmr_url = preferences.getString("dmr_url", "");

    logSerial("Loaded DMR config from storage");
    logSerial("Callsign: " + dmr_callsign);
    logSerial("DMR ID: " + String(dmr_id));
    logSerial("Server: " + dmr_server);
    logSerial("ESSID: " + String(dmr_essid));
  }

  // Load WiFi alternate settings
  // Load alternate WiFi networks
  for (int i = 0; i < 5; i++) {
    String labelKey = "wifi" + String(i) + "_label";
    String ssidKey = "wifi" + String(i) + "_ssid";
    String passKey = "wifi" + String(i) + "_pass";
    wifiNetworks[i].label = preferences.getString(labelKey.c_str(), wifiNetworks[i].label);
    wifiNetworks[i].ssid = preferences.getString(ssidKey.c_str(), "");
    wifiNetworks[i].password = preferences.getString(passKey.c_str(), "");
  }

  // Load hostname setting
  device_hostname = preferences.getString("hostname", MDNS_HOSTNAME);
  logSerial("Hostname: " + device_hostname);

  // Load verbose logging setting
  verbose_logging = preferences.getBool("verbose_log", false);
  logSerial("Verbose logging: " + String(verbose_logging ? "enabled" : "disabled"));

  // Load NTP timezone settings
  ntp_timezone_offset = preferences.getLong("ntp_tz_offset", NTP_TIMEZONE_OFFSET);
  ntp_daylight_offset = preferences.getLong("ntp_dst_offset", NTP_DAYLIGHT_OFFSET);
  logSerial("NTP Timezone offset: " + String(ntp_timezone_offset) + "s, DST offset: " + String(ntp_daylight_offset) + "s");

  // Load web username
  web_username = preferences.getString("web_username", WEB_USERNAME);
  if (web_username.length() == 0) {
    web_username = WEB_USERNAME;  // Fallback to default if empty
  }

  // Load web password
  web_password = preferences.getString("web_password", WEB_PASSWORD);
  if (web_password.length() == 0) {
    web_password = WEB_PASSWORD;  // Fallback to default if empty
  }
  logSerial("Web authentication: enabled (user/password: " + web_username + "/" + web_password + ")");

  // Load mode enable/disable settings (use config.h defaults if not saved)
  mode_dmr_enabled = preferences.getBool("mode_dmr", DEFAULT_MODE_DMR);
  mode_dstar_enabled = preferences.getBool("mode_dstar", DEFAULT_MODE_DSTAR);
  mode_ysf_enabled = preferences.getBool("mode_ysf", DEFAULT_MODE_YSF);
  mode_p25_enabled = preferences.getBool("mode_p25", DEFAULT_MODE_P25);
  mode_nxdn_enabled = preferences.getBool("mode_nxdn", DEFAULT_MODE_NXDN);
  mode_pocsag_enabled = preferences.getBool("mode_pocsag", DEFAULT_MODE_POCSAG);
  logSerial("Mode status - DMR: " + String(mode_dmr_enabled ? "ON" : "OFF") + " | D-Star: " + String(mode_dstar_enabled ? "ON" : "OFF") + " | YSF: " + String(mode_ysf_enabled ? "ON" : "OFF") + " | P25: " + String(mode_p25_enabled ? "ON" : "OFF") + " | NXDN: " + String(mode_nxdn_enabled ? "ON" : "OFF") + " | POCSAG: " + String(mode_pocsag_enabled ? "ON" : "OFF"));

  // Load modem type
  modem_type = preferences.getString("modem_type", DEFAULT_MODEM_TYPE);
  logSerial("Modem type: " + modem_type);

  preferences.end();
}

void saveConfig() {
  preferences.begin("mmdvm", false);

  preferences.putString("dmr_callsign", dmr_callsign);
  preferences.putUInt("dmr_id", dmr_id);
  preferences.putString("dmr_server", dmr_server);
  preferences.putString("dmr_password", dmr_password);
  preferences.putUChar("dmr_essid", dmr_essid);

  // Save additional config options
  preferences.putUInt("dmr_rx_freq", dmr_rx_freq);
  preferences.putUInt("dmr_tx_freq", dmr_tx_freq);
  preferences.putUChar("dmr_power", dmr_power);
  preferences.putUChar("dmr_cc", dmr_color_code);
  preferences.putFloat("dmr_lat", dmr_latitude);
  preferences.putFloat("dmr_lon", dmr_longitude);
  preferences.putInt("dmr_height", dmr_height);
  preferences.putString("dmr_location", dmr_location);
  preferences.putString("dmr_desc", dmr_description);
  preferences.putString("dmr_url", dmr_url);

  // Save alternate WiFi networks
  for (int i = 0; i < 5; i++) {
    String labelKey = "wifi" + String(i) + "_label";
    String ssidKey = "wifi" + String(i) + "_ssid";
    String passKey = "wifi" + String(i) + "_pass";
    preferences.putString(labelKey.c_str(), wifiNetworks[i].label);
    preferences.putString(ssidKey.c_str(), wifiNetworks[i].ssid);
    preferences.putString(passKey.c_str(), wifiNetworks[i].password);
  }

  // Save hostname
  preferences.putString("hostname", device_hostname);

  // Save verbose logging
  preferences.putBool("verbose_log", verbose_logging);

  // Save web credentials
  preferences.putString("web_username", web_username);
  preferences.putString("web_password", web_password);

  // Save mode enable/disable settings
  preferences.putBool("mode_dmr", mode_dmr_enabled);
  preferences.putBool("mode_dstar", mode_dstar_enabled);
  preferences.putBool("mode_ysf", mode_ysf_enabled);
  preferences.putBool("mode_p25", mode_p25_enabled);
  preferences.putBool("mode_nxdn", mode_nxdn_enabled);
  preferences.putBool("mode_pocsag", mode_pocsag_enabled);

  // Save modem type
  preferences.putString("modem_type", modem_type);

  preferences.end();
  logSerial("Configuration saved to storage");
}

// ===== Web Server Setup =====
void setupWebServer() {
  // Main pages
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/serialmonitor", handleMonitor);
  server.on("/wificonfig", handleConfig);
  server.on("/modeconfig", handleDMRConfig);
  server.on("/admin", handleAdmin);

  // Configuration handlers
  server.on("/saveconfig", HTTP_POST, handleSaveConfig);
  server.on("/savedmrconfig", HTTP_POST, handleSaveDMRConfig);
  server.on("/savemodes", HTTP_POST, handleSaveModes);
  server.on("/resetconfig", handleResetConfig);
  server.on("/confirmreset", HTTP_POST, handleConfirmReset);

  // Data endpoints
  server.on("/logs", handleGetLogs);
  server.on("/statusdata", handleStatusData);     // Status page data
  server.on("/wifiscan", handleWifiScan);
  server.on("/dmr-activity", handleDMRActivity);  // Live DMR activity for home page
  server.on("/dmr-slot1", handleDMRSlot1);        // DMR Slot 1 activity
  server.on("/dmr-slot2", handleDMRSlot2);        // DMR Slot 2 activity
  server.on("/dmr-history", handleDMRHistory);    // Recent DMR activity history
  server.on("/system-status", handleSystemStatus); // System status for auto-refresh

  // Admin actions
  server.on("/clearlogs", HTTP_POST, handleClearLogs);
  server.on("/save-hostname", HTTP_POST, handleSaveHostname);
  server.on("/save-verbose", HTTP_POST, handleSaveVerbose);
  server.on("/save-timezone", HTTP_POST, handleSaveTimezone);
  server.on("/save-username", HTTP_POST, handleSaveUsername);
  server.on("/save-password", HTTP_POST, handleSavePassword);
  server.on("/reboot", HTTP_POST, handleReboot);
  server.on("/restart-services", HTTP_POST, handleRestartServices);
  server.on("/export-config", handleExportConfig);
  server.on(
    "/import-config", HTTP_POST, []() {}, handleImportConfig);
  server.on("/showprefs", handleShowPreferences);
  server.on("/test-mmdvm", HTTP_POST, handleTestMmdvm);
  server.on("/cleanup-prefs", HTTP_POST, handleCleanupPreferences);
  server.on("/download-update", HTTP_POST, handleDownloadUpdate);
  server.on(
    "/upload-firmware", HTTP_POST, []() {}, handleUploadFirmware);
  server.on("/flash-firmware", HTTP_POST, handleFlashFirmware);

  server.begin();
  logSerial("Web server started.");
}

// ===== Status LED Control =====
void setLEDMode(LED_MODE mode) {
  currentLedMode = mode;
  lastLedToggle = millis();

  // Set initial state based on mode
  if (mode == LED_MODE::STEADY) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    ledState = true;
  } else if (mode == LED_MODE::OFF) {
    digitalWrite(STATUS_LED_PIN, LOW);
    ledState = false;
  }
}

void updateStatusLED() {
  unsigned long currentMillis = millis();
  unsigned long interval;

  switch (currentLedMode) {
    case LED_MODE::OFF:
      // LED stays off
      break;

    case LED_MODE::STEADY:
      // LED stays on
      break;

    case LED_MODE::FAST_BLINK:
      // Fast blink (200ms interval) - connecting to WiFi
      interval = 200;
      if (currentMillis - lastLedToggle >= interval) {
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState ? HIGH : LOW);
        lastLedToggle = currentMillis;
      }
      break;

    case LED_MODE::SLOW_BLINK:
      // Slow blink (1000ms interval) - Access Point mode
      interval = 1000;
      if (currentMillis - lastLedToggle >= interval) {
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState ? HIGH : LOW);
        lastLedToggle = currentMillis;
      }
      break;
  }
}

// ===== DMR User Information Lookup Functions =====

// Enhanced user info lookup - checks cache first, then API
String lookupUserInfo(uint32_t dmrId) {
  if (dmrId == 0) return "";
  
  // Check cache first
  String cached = getCachedUserInfo(dmrId);
  if (cached.length() > 0) {
    return cached;
  }
  
  // Not in cache, try API lookup
  String userInfo = lookupUserInfoAPI(dmrId);
  
  // Cache the result (even if empty to avoid repeated failed lookups)
  if (userInfo.length() > 0) {
    cacheUserInfo(dmrId, userInfo);
  }
  
  return userInfo;
}

// Legacy callsign lookup function - checks cache first, then API
String lookupCallsign(uint32_t dmrId) {
  if (dmrId == 0) return "";
  
  // Try enhanced lookup first
  String userInfo = lookupUserInfo(dmrId);
  if (userInfo.length() > 0) {
    int pipeIndex = userInfo.indexOf('|');
    return (pipeIndex > 0) ? userInfo.substring(0, pipeIndex) : userInfo;
  }
  
  // Fallback to legacy cache
  String cached = getCachedCallsign(dmrId);
  if (cached.length() > 0) {
    return cached;
  }
  
  // Not in cache, try API lookup
  String callsign = lookupCallsignAPI(dmrId);
  
  // Cache the result (even if empty to avoid repeated failed lookups)
  if (callsign.length() > 0) {
    cacheCallsign(dmrId, callsign);
  }
  
  return callsign;
}

// Check if user info is in cache
String getCachedUserInfo(uint32_t dmrId) {
  for (int i = 0; i < DMR_USER_CACHE_SIZE; i++) {
    if (userCache[i].dmrId == dmrId && userCache[i].userInfo.length() > 0) {
      return userCache[i].userInfo;
    }
  }
  return "";
}

// Add user info to cache (circular buffer)
void cacheUserInfo(uint32_t dmrId, String userInfo) {
  userCache[userCacheIndex].dmrId = dmrId;
  userCache[userCacheIndex].userInfo = userInfo;
  userCache[userCacheIndex].timestamp = millis();
  userCacheIndex = (userCacheIndex + 1) % DMR_USER_CACHE_SIZE;
}

// Check if callsign is in legacy cache
String getCachedCallsign(uint32_t dmrId) {
  for (int i = 0; i < DMR_CALLSIGN_CACHE_SIZE; i++) {
    if (callsignCache[i].dmrId == dmrId && callsignCache[i].callsign.length() > 0) {
      return callsignCache[i].callsign;
    }
  }
  return "";
}

// Add callsign to legacy cache (circular buffer)
void cacheCallsign(uint32_t dmrId, String callsign) {
  callsignCache[callsignCacheIndex].dmrId = dmrId;
  callsignCache[callsignCacheIndex].callsign = callsign;
  callsignCache[callsignCacheIndex].timestamp = millis();
  callsignCacheIndex = (callsignCacheIndex + 1) % DMR_CALLSIGN_CACHE_SIZE;
}

// Enhanced user info lookup via RadioID.net API
String lookupUserInfoAPI(uint32_t dmrId) {
  if (!wifiConnected) {
    return "";
  }
  
  HTTPClient http;
  String url = String(DMR_API_URL) + String(dmrId);
  
  http.begin(url);
  http.setTimeout(DMR_API_TIMEOUT);  // API timeout from config.h
  
  int httpCode = http.GET();
  String userInfo = "";
  
  if (httpCode == 200) {
    String payload = http.getString();
    
    // RadioID.net returns JSON: {"count":1,"results":[{"id":2041152,"callsign":"PA3ANG","fname":"John","name":"John","city":"Amsterdam","country":"Netherlands",...}]}
    // Parse multiple fields: callsign, name/fname, city, country
    String callsign = "";
    String name = "";
    String city = "";
    String country = "";
    
    // Extract callsign
    int csIndex = payload.indexOf("\"callsign\":\"");
    if (csIndex > 0) {
      csIndex += 12;  // Length of "callsign":"
      int endIndex = payload.indexOf("\"", csIndex);
      if (endIndex > csIndex) {
        callsign = payload.substring(csIndex, endIndex);
      }
    }
    
    // Extract name (prefer 'name' over 'fname')
    int nameIndex = payload.indexOf("\"name\":\"");
    if (nameIndex > 0) {
      nameIndex += 8;  // Length of "name":"
      int endIndex = payload.indexOf("\"", nameIndex);
      if (endIndex > nameIndex) {
        name = payload.substring(nameIndex, endIndex);
        if (name == "null" || name.length() == 0) {
          // Try fname if name is null/empty
          int fnameIndex = payload.indexOf("\"fname\":\"");
          if (fnameIndex > 0) {
            fnameIndex += 9;  // Length of "fname":"
            int fendIndex = payload.indexOf("\"", fnameIndex);
            if (fendIndex > fnameIndex) {
              name = payload.substring(fnameIndex, fendIndex);
            }
          }
        }
      }
    }
    
    // Extract city
    int cityIndex = payload.indexOf("\"city\":\"");
    if (cityIndex > 0) {
      cityIndex += 8;  // Length of "city":"
      int endIndex = payload.indexOf("\"", cityIndex);
      if (endIndex > cityIndex) {
        city = payload.substring(cityIndex, endIndex);
        if (city == "null") city = "";
      }
    }
    
    // Extract country
    int countryIndex = payload.indexOf("\"country\":\"");
    if (countryIndex > 0) {
      countryIndex += 11;  // Length of "country":"
      int endIndex = payload.indexOf("\"", countryIndex);
      if (endIndex > countryIndex) {
        country = payload.substring(countryIndex, endIndex);
        if (country == "null") country = "";
      }
    }
    
    // Build userInfo string: "callsign|name|city|country"
    if (callsign.length() > 0) {
      userInfo = callsign;
      if (name.length() > 0 || city.length() > 0 || country.length() > 0) {
        userInfo += "|" + name + "|" + city + "|" + country;
      }
    }
  } else if (httpCode > 0) {
    logSerial("User info lookup failed: HTTP " + String(httpCode));
  }
  
  http.end();
  return userInfo;
}

// Legacy callsign lookup via RadioID.net API
String lookupCallsignAPI(uint32_t dmrId) {
  // Use enhanced lookup and extract just the callsign
  String userInfo = lookupUserInfoAPI(dmrId);
  if (userInfo.length() > 0) {
    int pipeIndex = userInfo.indexOf('|');
    return (pipeIndex > 0) ? userInfo.substring(0, pipeIndex) : userInfo;
  }
  return "";
}

// Helper function to get current timestamp in HH:MM:SS format
String getCurrentTimestamp() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    // We have NTP time
    char timeStr[9];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    return String(timeStr);
  } else {
    // Fallback to uptime if NTP not available
    unsigned long totalSeconds = millis() / 1000;
    int hours = (totalSeconds / 3600) % 24;
    int minutes = (totalSeconds / 60) % 60;
    int seconds = totalSeconds % 60;

    String timestamp = "";
    if (hours < 10) timestamp += "0";
    timestamp += String(hours) + ":";
    if (minutes < 10) timestamp += "0";
    timestamp += String(minutes) + ":";
    if (seconds < 10) timestamp += "0";
    timestamp += String(seconds);
    return timestamp;
  }
}

// Add DMR transmission to history
void addDMRHistory(uint32_t srcId, String srcCallsign, String srcName, String srcLocation, uint32_t dstId, bool isGroup, uint32_t duration, uint8_t ber, uint8_t rssi, uint8_t slotNo) {
  // Debug log
  logSerial("Adding to history: " + srcCallsign + " (" + String(srcId) + ") -> " + (isGroup ? "TG" : "") + String(dstId) + " Duration: " + String(duration) + "s");

  // Get current timestamp (NTP time or fallback to uptime)
  String timestamp = getCurrentTimestamp();

  // Add to circular buffer
  dmrHistory[dmrHistoryIndex].timestamp = timestamp;
  dmrHistory[dmrHistoryIndex].srcId = srcId;
  dmrHistory[dmrHistoryIndex].srcCallsign = srcCallsign;
  dmrHistory[dmrHistoryIndex].srcName = srcName;
  dmrHistory[dmrHistoryIndex].srcLocation = srcLocation;
  dmrHistory[dmrHistoryIndex].dstId = dstId;
  dmrHistory[dmrHistoryIndex].isGroup = isGroup;
  dmrHistory[dmrHistoryIndex].duration = duration;
  dmrHistory[dmrHistoryIndex].ber = ber;
  dmrHistory[dmrHistoryIndex].rssi = rssi;
  dmrHistory[dmrHistoryIndex].slotNo = slotNo;
  
  dmrHistoryIndex = (dmrHistoryIndex + 1) % DMR_HISTORY_SIZE;
}

#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
// Helper functions for Ethernet/SD status display
String getEthIPAddress() {
  return eth_connected ? ETH.localIP().toString() : "0.0.0.0";
}

String getEthMACAddress() {
  return eth_connected ? ETH.macAddress() : "00:00:00:00:00:00";
}

int getEthLinkSpeed() {
  return eth_connected ? ETH.linkSpeed() : 0;
}

bool getEthFullDuplex() {
  return eth_connected ? ETH.fullDuplex() : false;
}

String getEthGatewayIP() {
  return eth_connected ? ETH.gatewayIP().toString() : "0.0.0.0";
}

uint64_t getSDCardSize() {
  return sdCardAvailable ? SD.cardSize() : 0;
}

uint64_t getSDUsedBytes() {
  return sdCardAvailable ? SD.usedBytes() : 0;
}

uint8_t getSDCardType() {
  return sdCardAvailable ? sdCardType : 0;  // Return cached value
}
#endif

// ===== OLED Display Functions =====
#if ENABLE_OLED
void setupOLED() {
  // Create mutex for display access protection
  displayMutex = xSemaphoreCreateMutex();
  if (displayMutex == NULL) {
    logSerial("OLED: Failed to create display mutex!");
  } else {
    logSerial("OLED: Display mutex created");
  }

  // Initialize I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    logSerial("OLED: SSD1306 allocation failed!");
    return;
  }
  logSerial("OLED: Display initialized successfully");

  // Display boot logos
  //
  //Bitmap logo first
  displayBitmap();
  logSerial("OLED: Display logo bitmap");
  delay(5000);  // Show logo for 2 seconds
  //
  // Then ESP32 logo
  //displayESP32Logo();
  //logSerial("OLED: Showing ESP32 logo");
  //delay(2000);  // Show logo for 2 seconds
  //
  // boot logo last
  displayBootLogo();
  logSerial("OLED: Showing boot screen");
}


void displayBitmap(void) {
  display.clearDisplay();

  display.drawBitmap(
    (display.width()  - LOGO_WIDTH ) / 2,
    (display.height() - LOGO_HEIGHT) / 2,
    logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(1000);
}

void displayESP32Logo() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Draw decorative top border
  for (int i = 0; i < OLED_WIDTH; i += 4) {
    display.drawPixel(i, 0, SSD1306_WHITE);
    display.drawPixel(i + 1, 1, SSD1306_WHITE);
  }

  // ESP32 text - large and centered
  display.setTextSize(3);
  String esp32Text = "ESP32";
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(esp32Text, 0, 0, &x1, &y1, &w, &h);
  int16_t x = (OLED_WIDTH - w) / 2;
  display.setCursor(x, 15);
  display.println(esp32Text);

  // MMDVM Hotspot text - smaller, centered
  display.setTextSize(1);
  String mmdvmText = "MMDVM Hotspot";
  display.getTextBounds(mmdvmText, 0, 0, &x1, &y1, &w, &h);
  x = (OLED_WIDTH - w) / 2;
  display.setCursor(x, 45);
  display.println(mmdvmText);

  // Draw decorative bottom border
  for (int i = 0; i < OLED_WIDTH; i += 4) {
    display.drawPixel(i, OLED_HEIGHT - 2, SSD1306_WHITE);
    display.drawPixel(i + 1, OLED_HEIGHT - 1, SSD1306_WHITE);
  }

  display.display();
}

void displayBootLogo() {
  display.clearDisplay();

  // Set text properties
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Title with callsign - centered
  String title = "ESP32 Hotspot";
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  int16_t x = (OLED_WIDTH - w) / 2;
  display.setCursor(x, 0);
  display.println(title);

  // Draw a line
  display.drawLine(0, 10, OLED_WIDTH, 10, SSD1306_WHITE);

  // Firmware version
  display.setCursor(0, 14);
  display.println("Version:");
  display.setCursor(0, 24);
  display.println(FIRMWARE_VERSION);

  // Authors
  display.setCursor(0, 34);
  display.println("Made by:");
  display.setCursor(0, 44);
  display.println("PD2EMC & PD8JO");

  // Status
  display.setCursor(0, 55);
  display.println("Booting...");

  display.display();

  logSerial("OLED: Boot logo displayed");
}

void updateBootStatus(String status) {
#if ENABLE_OLED
  // Update only the status line at the bottom of boot screen
  // Clear the status area (bottom line)
  display.fillRect(0, 55, OLED_WIDTH, 9, SSD1306_BLACK);

  // Display new status
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 55);
  display.println(status);

  display.display();

  logSerial("OLED: " + status);
#endif
}

void updateOLEDStatus() {
  // Try to acquire mutex to prevent SPI conflicts with Ethernet
  if (displayMutex != NULL) {
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
      // Could not acquire mutex in 50ms, skip this update to avoid blocking
      logSerial("OLED: Skipped update - mutex busy");
      return;
    }
  }

  // Clear entire display for clean look
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // ===== TOP LINE: Date on left, Time centered, Icons on right =====
  // Get current time
  struct tm timeinfo;
  String dateStr = "";
  String timeStr = "";
  if (getLocalTime(&timeinfo)) {
    char dateBuf[8];
    char timeBuf[8];
    strftime(dateBuf, sizeof(dateBuf), "%d/%m", &timeinfo);
    strftime(timeBuf, sizeof(timeBuf), "%H:%M", &timeinfo);
    dateStr = String(dateBuf);
    timeStr = String(timeBuf);
  } else {
    dateStr = "--/--";
    timeStr = "--:--";
  }

  // Display date on the left
  display.setCursor(0, 0);
  display.print(dateStr);

  // Display time centered
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  int16_t timeX = (OLED_WIDTH - w) / 2;
  display.setCursor(timeX, 0);
  display.print(timeStr);

  // Draw icons on the right side of top line (right to left)
  int iconX = OLED_WIDTH - ICON_WIDTH; // Start from right edge

  // Draw network icons - show both if both connected
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
  if (eth_connected && wifiConnected) {
    // Both connected - show both icons
    display.drawBitmap(iconX, 0, icon_ethernet, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
    iconX -= (ICON_WIDTH + 2);
    display.drawBitmap(iconX, 0, icon_wifi, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
    iconX -= (ICON_WIDTH + 2);
  } else if (eth_connected) {
    // Ethernet only
    display.drawBitmap(iconX, 0, icon_ethernet, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
    iconX -= (ICON_WIDTH + 2);
  } else if (wifiConnected) {
    // WiFi only
    display.drawBitmap(iconX, 0, icon_wifi, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
    iconX -= (ICON_WIDTH + 2);
  }
#else
  if (wifiConnected) {
    display.drawBitmap(iconX, 0, icon_wifi, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
    iconX -= (ICON_WIDTH + 2);
  }
#endif

  // Draw antenna/DMR icon (left of network icons)
  if (mode_dmr_enabled && dmrLoggedIn) {
    display.drawBitmap(iconX, 0, icon_antenna, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
  }

  // Draw a line below top row
  display.drawLine(0, 10, OLED_WIDTH, 10, SSD1306_WHITE);

  // DMR Activity Section (between the two lines)
  // Check for active DMR transmission
  bool activityDisplayed = false;

  // Check if both slots are active
  bool bothSlotsActive = dmrActivity[0].active && dmrActivity[1].active;

  // Determine which slot to display
  int slotToDisplay = -1;
  if (bothSlotsActive) {
    // Both active - alternate between them
    slotToDisplay = oledActiveSlot;
    oledActiveSlot = (oledActiveSlot == 0) ? 1 : 0; // Toggle for next update
  } else if (dmrActivity[1].active) {
    // Only Slot 2 active (prioritize Slot 2)
    slotToDisplay = 1;
  } else if (dmrActivity[0].active) {
    // Only Slot 1 active
    slotToDisplay = 0;
  }

  // Display the selected slot
  if (slotToDisplay >= 0) {
    int i = slotToDisplay;

    // Callsign - LARGE and prominent (2x size, centered)
    display.setTextSize(2);
    String callsign = (dmrActivity[i].srcCallsign.length() > 0) ? dmrActivity[i].srcCallsign : "Unknown";
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(callsign, 0, 0, &x1, &y1, &w, &h);
    int16_t x = (OLED_WIDTH - w) / 2;
    display.setCursor(x, 14);
    display.println(callsign);

    // Duration (small text, below callsign)
    display.setTextSize(1);
    display.setCursor(0, 32);
    unsigned long duration = (millis() - dmrActivity[i].startTime) / 1000;
    display.print("Duration: ");
    display.print(duration);
    display.println("s");

    // DMR ID -> Talkgroup with slot indicator at the end (small text)
    display.setCursor(0, 42);
    display.print(dmrActivity[i].srcId);
    display.print(" -> TG ");
    display.print(dmrActivity[i].dstId);
    display.print(" [S");
    display.print(dmrActivity[i].slotNo);
    display.print("]");

    activityDisplayed = true;
  }

  // If no active transmission, show idle state
  if (!activityDisplayed) {
    display.setCursor(0, 20);

    // Check if any mode is enabled
    bool anyModeEnabled = mode_dmr_enabled || mode_dstar_enabled || mode_ysf_enabled ||
                          mode_p25_enabled || mode_nxdn_enabled || mode_pocsag_enabled;

    if (!anyModeEnabled) {
      // No modes activated - center the text
      String line1 = "No mode activated";
      String line2 = "Enable mode in web";

      int16_t x1, y1;
      uint16_t w, h;

      // Center first line
      display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
      int16_t x = (OLED_WIDTH - w) / 2;
      display.setCursor(x, 20);
      display.println(line1);

      // Center second line
      display.getTextBounds(line2, 0, 0, &x1, &y1, &w, &h);
      x = (OLED_WIDTH - w) / 2;
      display.setCursor(x, 30);
      display.println(line2);
    } else if (mode_dmr_enabled) {
      // DMR mode is enabled - show status
      if (dmrLoggedIn) {
        display.println("DMR: Listening");

        // Show current talkgroup if available
        if (currentTalkgroup > 0) {
          display.setCursor(0, 30);
          display.print("TG: ");
          display.println(currentTalkgroup);
        }

        // Show last caller callsign (check both slots for most recent)
        String lastCaller = "";
        unsigned long mostRecentTime = 0;

        for (int i = 0; i < 2; i++) {
          if (dmrActivity[i].srcCallsign.length() > 0 && dmrActivity[i].lastUpdate > mostRecentTime) {
            lastCaller = dmrActivity[i].srcCallsign;
            mostRecentTime = dmrActivity[i].lastUpdate;
          }
        }

        if (lastCaller.length() > 0) {
          display.setCursor(0, 40);
          display.print("Last: ");
          display.println(lastCaller);
        }
      } else {
        // DMR enabled but not connected
        display.println("DMR Mode Active");
        display.setCursor(0, 30);
        display.print("Status: ");
        display.println(dmrLoginStatus);
      }
    } else {
      // Other mode is enabled (D-Star, YSF, P25, NXDN, POCSAG)
      // Show which mode(s) are enabled but not yet implemented
      display.println("Mode enabled:");
      display.setCursor(0, 30);
      if (mode_dstar_enabled) display.println("D-Star (N/A)");
      else if (mode_ysf_enabled) display.println("YSF (N/A)");
      else if (mode_p25_enabled) display.println("P25 (N/A)");
      else if (mode_nxdn_enabled) display.println("NXDN (N/A)");
      else if (mode_pocsag_enabled) display.println("POCSAG (N/A)");
    }
  }

  // ===== BOTTOM ROW: Cycling Network/Callsign info =====
  // Draw a line above bottom status
  display.drawLine(0, 50, OLED_WIDTH, 50, SSD1306_WHITE);

  display.setTextSize(1);

  // Determine what to show based on connections and cycle state
  bool hasWifi = wifiConnected;
  bool hasEth = false;
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
  hasEth = eth_connected;
#endif

  String bottomLine = "";
  bool centerText = false; // Flag to center callsign text

  if (hasWifi && hasEth) {
    // Both connected - cycle through 3 screens
    if (oledHeaderCycle == 0) {
      bottomLine = "WiFi: " + WiFi.localIP().toString();
    } else if (oledHeaderCycle == 1) {
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
      bottomLine = "ETH: " + ETH.localIP().toString();
#endif
    } else {
      bottomLine = dmr_callsign + " - ESP32 HS";
      centerText = true;
    }
  } else if (hasWifi) {
    // WiFi only - alternate between 2 screens
    if (oledHeaderCycle % 2 == 0) {
      bottomLine = "WiFi: " + WiFi.localIP().toString();
    } else {
      bottomLine = dmr_callsign + " - ESP32 HS";
      centerText = true;
    }
  } else if (hasEth) {
    // ETH only - alternate between 2 screens
    if (oledHeaderCycle % 2 == 0) {
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
      bottomLine = "ETH: " + ETH.localIP().toString();
#endif
    } else {
      bottomLine = dmr_callsign + " - ESP32 HS";
      centerText = true;
    }
  } else if (apMode) {
    // AP mode
    if (oledHeaderCycle % 2 == 0) {
      bottomLine = "AP: " + WiFi.softAPIP().toString();
    } else {
      bottomLine = dmr_callsign + " - ESP32 HS";
      centerText = true;
    }
  } else {
    // No network - alternate between warning and callsign
    if (oledHeaderCycle % 2 == 0) {
      bottomLine = "No Network";
      centerText = true;
    } else {
      bottomLine = dmr_callsign + " - ESP32 HS";
      centerText = true;
    }
  }

  // Center the text if it's the callsign, otherwise left-align
  if (centerText) {
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(bottomLine, 0, 0, &x1, &y1, &w, &h);
    int16_t x = (OLED_WIDTH - w) / 2;
    display.setCursor(x, 54);
  } else {
    display.setCursor(0, 54);
  }

  display.print(bottomLine);

  display.display();

  // Release mutex after display update is complete
  if (displayMutex != NULL) {
    xSemaphoreGive(displayMutex);
  }
}
#endif
