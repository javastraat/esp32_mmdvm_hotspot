/*
 * MMDVM DMR Protocol Module Implementation
 *
 * Handles:
 * - BrandMeister network login (RPTL/RPTK/RPTC state machine)
 * - Keepalive (RPTPING/MSTPONG)
 * - Incoming DMR network packets (DMRD)
 * - Modem serial DMR frame processing
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "mbedtls/md.h"
#include "sqlite3.h"
#include <SD.h>

#include "mmdvm/mmdvm_dmr.h"
#include "mmdvm/mmdvm_pocsag.h"
#include "system/system_logger.h"
#include "system/system_modem.h"
#include "system/service_mqtt.h"
#include "system/system_eth.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "system/system_firmware.h"
#include "include/config.h"
#include "system/system_oled.h"
#include "system/system_espnow.h"

// SD card status from system_sdcard.h
extern bool sdCardMounted;

// Forward declarations
String lookupDmrUserInfo(uint32_t dmrId, String &source);
static void parseDmrUserInfo(const String &userInfo, String &callsign, String &name,
                              String &city, String &state, String &country);
static QueueHandle_t dmrLookupQueue = nullptr;
static String tryExtractTA(const uint8_t* frame33, bool isNewTx);

// Escape a string for safe embedding in a JSON value
static String jsonStr(const String &s) {
  String out;
  out.reserve(s.length() + 4);
  for (int i = 0; i < (int)s.length(); i++) {
    char c = s[i];
    if      (c == '"')  out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else                out += c;
  }
  return out;
}

// MQTT topics
extern String mqttDmrTaskTopic;
extern String mqttModemTaskTopic;

// Settings from main .ino
extern String userCallsign;
extern uint32_t userDmrId;
extern uint8_t userDmrSsid;
extern String dmrServer;
extern uint16_t dmrPort;
extern uint16_t dmrLocalPort;
extern String dmrPassword;
extern String hotspotCallsign;
extern String hotspotLatitude;
extern String hotspotLongitude;
extern int hotspotHeight;
extern String hotspotLocation;
extern String hotspotDescription;
extern String hotspotUrl;
extern uint32_t dmrRxFreq;
extern uint32_t dmrTxFreq;
extern uint8_t dmrColorCode;
extern uint8_t dmrRfPower;
extern bool sdcardEnabled;
extern String dmrApiUrl;
extern uint16_t dmrApiTimeout;
extern bool dmrServerEspNow;

// Task handle
TaskHandle_t dmrTaskHandle = NULL;

// DMR network state (accessible by other tasks)
volatile DMR_STATE dmrState = DMR_STATE::DISCONNECTED;
volatile bool dmrLoggedIn = false;
String dmrLoginStatus = "Not Connected";

// File-scope UDP socket (only used by dmrTask)
static WiFiUDP dmrUdp;

// Login state tracking
static uint8_t dmrSalt[4];
static unsigned long lastLoginAttempt = 0;
static int loginAttempts = 0;
static unsigned long lastKeepalive = 0;

// Network→RF transmission state
volatile bool dmrTxActive = false;       // Modem is in DMR TX mode (extern in mmdvm_dmr.h)
// (lastDMRFrameTime removed — TX start/stop is now driven by netRxActive session)

// RF→Network receive state
volatile bool dmrRfRxActive = false;     // Modem is receiving a DMR signal from RF
static unsigned long dmrRfRxLastFrameMs = 0; // Timestamp of last CMD_DMR_DATA frame from modem
static uint32_t netRxSrcId = 0;          // Current network RX source ID
static uint32_t netRxDstId = 0;          // Current network RX dest ID
static uint8_t netRxSlot = 0;            // Current slot being received

// Separate network-layer RX tracking (independent of modem TX state).
// isNewTx uses this so that POCSAG holding the modem doesn't re-trigger
// the Net→RF log/MQTT for every packet of an ongoing transmission.
static bool netRxActive = false;
static unsigned long netRxLastPktTime = 0;
static const unsigned long NET_RX_IDLE_MS = 3000; // 3s gap = new transmission
static String netRxCallsign = "";

// DMR TX frame buffer for pacing (circular buffer)
#define DMR_TX_BUFFER_SIZE 64
typedef struct {
  uint8_t data[34];
  uint8_t cmd;
  bool valid;
} DmrTxFrame;
static DmrTxFrame dmrTxBuffer[DMR_TX_BUFFER_SIZE];
static volatile int dmrTxHead = 0;
static volatile int dmrTxTail = 0;


// ===== Helper: get the DMR ID to send (with ESSID if set) =====
static uint32_t getDmrIdToSend()
{
  uint32_t id = userDmrId;
  if (userDmrSsid > 0)
  {
    id = userDmrId * 100 + userDmrSsid;
  }
  return id;
}

// ===== Task Init =====
void initDmrTask()
{
  BaseType_t result = xTaskCreatePinnedToCore(
      dmrTask,
      "DMR Task",
      MMDVM_DMR_STACK,
      NULL,
      MMDVM_DMR_PRIORITY,
      &dmrTaskHandle,
      1 // Run on core 1
  );
  if (result != pdPASS)
    log_e("[DMR] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());
}

// ===== DMR Network: Send login packet (RPTL) =====
void connectToDMRNetwork()
{
  dmrLoginStatus = "Connecting...";
  dmrLoggedIn = false;
  dmrState = DMR_STATE::WAITING_LOGIN;
  lastLoginAttempt = millis();

  uint32_t idToSend = getDmrIdToSend();

  addLogMessage("[DMR] Connecting to " + dmrServer + ":" + String(dmrPort));
  addLogMessage("[DMR] Callsign: " + userCallsign + " ID: " + String(idToSend));
  publishMqtt(mqttDmrTaskTopic.c_str(),
              "{\"event\":\"connecting\",\"server\":\"" + jsonStr(dmrServer) +
              "\",\"port\":" + String(dmrPort) +
              ",\"callsign\":\"" + jsonStr(userCallsign) +
              "\",\"id\":" + String(idToSend) + "}");

  // RPTL packet: "RPTL" (4 bytes) + DMR_ID (4 bytes big-endian)
  uint8_t loginPacket[8];
  memcpy(loginPacket, "RPTL", 4);
  loginPacket[4] = (idToSend >> 24) & 0xFF;
  loginPacket[5] = (idToSend >> 16) & 0xFF;
  loginPacket[6] = (idToSend >> 8) & 0xFF;
  loginPacket[7] = idToSend & 0xFF;

  dmrUdp.beginPacket(dmrServer.c_str(), dmrPort);
  dmrUdp.write(loginPacket, 8);
  dmrUdp.endPacket();

  addLogMessage("[DMR] Login packet sent (RPTL)");
}

// ===== DMR Network: Send auth packet (RPTK) with SHA256(salt + password) =====
void sendDMRAuth(const uint8_t *salt)
{
  uint32_t idToSend = getDmrIdToSend();

  // Calculate SHA256(salt + password)
  size_t passLen = dmrPassword.length();
  size_t inputLen = 4 + passLen;
  uint8_t *input = new uint8_t[inputLen];

  memcpy(input, salt, 4);
  memcpy(input + 4, dmrPassword.c_str(), passLen);

  uint8_t hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, input, inputLen);
  mbedtls_md_finish(&ctx, hash);
  mbedtls_md_free(&ctx);

  delete[] input;

  // RPTK packet: "RPTK" (4) + ID (4 big-endian) + SHA256 hash (32) = 40 bytes
  uint8_t authPacket[40];
  memcpy(authPacket, "RPTK", 4);
  authPacket[4] = (idToSend >> 24) & 0xFF;
  authPacket[5] = (idToSend >> 16) & 0xFF;
  authPacket[6] = (idToSend >> 8) & 0xFF;
  authPacket[7] = idToSend & 0xFF;
  memcpy(authPacket + 8, hash, 32);

  dmrUdp.beginPacket(dmrServer.c_str(), dmrPort);
  dmrUdp.write(authPacket, 40);
  dmrUdp.endPacket();

  addLogMessage("[DMR] Auth packet sent (RPTK, 40 bytes)");
}

// ===== DMR Network: Send config packet (RPTC) =====
void sendDMRConfig()
{
  uint32_t idToSend = getDmrIdToSend();

  // Build config string (294 bytes) per MMDVMHost writeConfig() format:
  // %-8.8s%09u%09u%02u%02u%8.8s%9.9s%03d%-20.20s%-19.19s%c%-124.124s%-40.40s%-40.40s
  char configString[400];
  memset(configString, 0, sizeof(configString));

  char latitude[20];
  char longitude[20];
  snprintf(latitude, sizeof(latitude), "%08f", atof(hotspotLatitude.c_str()));
  snprintf(longitude, sizeof(longitude), "%09f", atof(hotspotLongitude.c_str()));

  unsigned int power = dmrRfPower;
  if (power > 99) power = 99;
  int height = hotspotHeight;
  if (height > 999) height = 999;

  snprintf(configString, sizeof(configString),
           "%-8.8s%09u%09u%02u%02u%8.8s%9.9s%03d%-20.20s%-19.19s%c%-124.124s%-40.40s%-40.40s",
           userCallsign.c_str(),       // Callsign (8 chars)
           dmrRxFreq,                  // RX Frequency (9 digits)
           dmrTxFreq,                  // TX Frequency (9 digits)
           power,                      // Power (2 digits)
           (unsigned int)dmrColorCode, // Color Code (2 digits)
           latitude,                   // Latitude (8 chars)
           longitude,                  // Longitude (9 chars)
           height,                     // Height (3 digits)
           hotspotLocation.c_str(),    // Location (20 chars)
           hotspotDescription.c_str(), // Description (19 chars)
           '4',                        // Slots ('4' = simplex)
           hotspotUrl.c_str(),         // URL (124 chars)
           firmwareVersion.c_str(),    // Version (40 chars)
           "MMDVM_MMDVM_HS");          // Software (40 chars)

  // RPTC packet: "RPTC" (4) + ID (4 big-endian) + config (294) = 302 bytes
  uint8_t configPacket[302];
  memset(configPacket, 0, sizeof(configPacket));

  memcpy(configPacket, "RPTC", 4);
  configPacket[4] = (idToSend >> 24) & 0xFF;
  configPacket[5] = (idToSend >> 16) & 0xFF;
  configPacket[6] = (idToSend >> 8) & 0xFF;
  configPacket[7] = idToSend & 0xFF;
  memcpy(configPacket + 8, configString, 294);

  dmrUdp.beginPacket(dmrServer.c_str(), dmrPort);
  dmrUdp.write(configPacket, 302);
  dmrUdp.endPacket();

  addLogMessage("[DMR] Config packet sent (RPTC, 302 bytes)");
}

// ===== DMR Network: Send keepalive (RPTPING) =====
void sendDMRKeepalive()
{
  uint32_t idToSend = getDmrIdToSend();

  // RPTPING packet: "RPTPING" (7) + ID (4 big-endian) = 11 bytes
  uint8_t keepalive[11];
  memcpy(keepalive, "RPTPING", 7);
  keepalive[7] = (idToSend >> 24) & 0xFF;
  keepalive[8] = (idToSend >> 16) & 0xFF;
  keepalive[9] = (idToSend >> 8) & 0xFF;
  keepalive[10] = idToSend & 0xFF;

  dmrUdp.beginPacket(dmrServer.c_str(), dmrPort);
  dmrUdp.write(keepalive, 11);
  dmrUdp.endPacket();
}

// ===== DMR Network: Process a validated DMRD packet =====
// Called from both the UDP receive path and the ESP-NOW receiver inject path.
// Does NOT forward over ESP-NOW (only the UDP path does that, to avoid loops).
static void processDMRDPacket(const uint8_t* packet, int len)
{
  uint8_t seqNo = packet[4];
  uint32_t srcId = (packet[5] << 16) | (packet[6] << 8) | packet[7];
  uint32_t dstId = (packet[8] << 16) | (packet[9] << 8) | packet[10];
  uint8_t slotBits = packet[15];
  uint8_t slotNo = (slotBits & 0x80) ? 2 : 1;
  bool isGroup = (slotBits & 0x40) != 0;

  // New transmission = different station, OR no network packet for 3s.
  // Using netRxActive/netRxLastPktTime (network layer) instead of dmrTxActive
  // (modem layer) avoids re-triggering the log/MQTT every time POCSAG holds
  // the modem and the 500ms modem-idle timeout fires mid-conversation.
  unsigned long nowPkt = millis();
  bool isNewTx = !netRxActive ||
                 (nowPkt - netRxLastPktTime > NET_RX_IDLE_MS) ||
                 netRxSrcId != srcId ||
                 netRxDstId != dstId;

  // Always update network RX tracking
  netRxActive = true;
  netRxLastPktTime = nowPkt;
  if (isNewTx) {
    netRxSrcId = srcId;
    netRxDstId = dstId;
    netRxSlot  = slotNo;
  }

  // Get user info — cache only (fast). If not cached, queue an async lookup.
  // The lookup task will update the OLED when it completes without blocking
  // the DMR task, which would otherwise delay keepalives and cause disconnects.
  String userInfo = getCachedDmrUserInfo(srcId);
  if (isNewTx) {
    if (!userInfo.isEmpty()) {
      addLogMessage("[DMR-LOOKUP] Found in cache: " + userInfo);
    } else if (dmrLookupQueue != nullptr) {
      // Skip if this ID was already queued recently (avoids duplicate lookups when
      // the same station calls back while the first async lookup is still running)
      static uint32_t lastQueuedId = 0;
      if (srcId != lastQueuedId) {
        xQueueSend(dmrLookupQueue, &srcId, 0);  // non-blocking, drop if queue full
        lastQueuedId = srcId;
      }
    }
  }
  String callsign, name, city, state, country;
  parseDmrUserInfo(userInfo, callsign, name, city, state, country);
  setDmrTxUserInfo(callsign, String(srcId), name, city, state, country);

  // Talker Alias: extract from embedded LC and show while DB lookup is pending.
  // tryExtractTA() maintains its own state machine across frames; it returns
  // non-empty text only when a new TA block completes (roughly once per 4 frames).
  {
      static String lastTa;
      if (isNewTx) lastTa = "";
      String ta = tryExtractTA(&packet[20], isNewTx);
      if (ta.length() > 0 && userInfo.isEmpty() && ta != lastTa) {
          setDmrTxUserInfo(ta, String(srcId), "", "", "", "");
          addLogMessage("[DMR-TA] Talker Alias: " + ta);
          lastTa = ta;
      }
  }

  // Log and publish once per new transmission (after user info is parsed)
  if (isNewTx) {
    addLogMessage("[DMR] Net→RF: " + String(srcId) + "→" +
                  (isGroup ? "TG" : "") + String(dstId) +
                  " slot=" + String(slotNo));
    if (userInfo.length() > 0)
      addLogMessage("[DMR] SRC UserInfo: " + userInfo);

    // Store callsign for the "ended" event
    netRxCallsign = callsign;

    // Publish full user info as JSON.
    // source=cache means data is complete; source=pending means user_info event follows.
    String json = "{\"event\":\"net2rf\""
                  ",\"src\":" + String(srcId) +
                  ",\"dst\":" + String(dstId) +
                  ",\"slot\":" + String(slotNo) +
                  ",\"group\":" + (isGroup ? "true" : "false") +
                  ",\"source\":\"" + (userInfo.isEmpty() ? "pending" : "cache") + "\"" +
                  ",\"callsign\":\"" + jsonStr(callsign) + "\"" +
                  ",\"name\":\"" + jsonStr(name) + "\"" +
                  ",\"city\":\"" + jsonStr(city) + "\"" +
                  ",\"state\":\"" + jsonStr(state) + "\"" +
                  ",\"country\":\"" + jsonStr(country) + "\"}";
    publishMqtt(mqttDmrTaskTopic.c_str(), json.c_str());
  }

  // Helper: enqueue one frame into the TX circular buffer
  uint8_t txCmd = (slotNo == 1) ? CMD_DMR_DATA1 : CMD_DMR_DATA2;
  auto enqueue = [&](const uint8_t* frameData33) -> bool {
    int nextHead = (dmrTxHead + 1) % DMR_TX_BUFFER_SIZE;
    if (nextHead == dmrTxTail) return false; // full
    DmrTxFrame &f = dmrTxBuffer[dmrTxHead];
    f.data[0] = 0x00;
    memcpy(&f.data[1], frameData33, 33);
    f.cmd   = txCmd;
    f.valid = true;
    dmrTxHead = nextHead;
    return true;
  };

  if (mmdvmReady)
  {
    // Queue every frame exactly once — LC header, voice, terminator.
    // The modem firmware's DMRTX state machine handles the protocol timing
    // and LC embedding on-air. Extra host-side injections add non-voice
    // frames that the radio hears as data bursts / audio glitches.
    enqueue(&packet[20]);
  }
}

// ===== DMR Network: Handle incoming UDP packets + ESP-NOW injections =====
void handleDMRNetwork()
{
#if ESPNOW_SENDER
  // Receiver mode: drain queued ESP-NOW DMRD packets each loop iteration.
  // The ESP-NOW receive callback (WiFi task context) enqueues; we process here
  // in the DMR task context — same as if the packet arrived via UDP.
  // No re-forwarding: processDMRDPacket() never calls espnowSendDmrNetPacket().
  if (espnowReceiverEnabled && espnowDmrNetQueue) {
    EspNowDmrNetPacket ep;
    while (xQueueReceive(espnowDmrNetQueue, &ep, 0) == pdTRUE) {
      if (ep.type == ESPNOW_TYPE_DMR_NET &&
          ep.len >= 55 && ep.len <= 60 &&
          memcmp(ep.data, "DMRD", 4) == 0) {
        processDMRDPacket(ep.data, (int)ep.len);
      }
    }
  }
#endif

  // In ESP-NOW relay mode the DMR data source is the ESP-NOW queue (drained above).
  // Skip BrandMeister UDP entirely — no socket read, no protocol parsing.
  if (dmrServerEspNow) return;

  int packetSize = dmrUdp.parsePacket();
  if (!packetSize)
    return;

  uint8_t packet[512];
  int len = dmrUdp.read(packet, sizeof(packet));
  if (len <= 0)
    return;

  // --- MSTNAK: Negative acknowledgment ---
  if (len >= 6 && memcmp(packet, "MSTNAK", 6) == 0)
  {
    dmrLoggedIn = false;
    dmrLoginStatus = "Login Failed";

    String stage;
    switch (dmrState)
    {
    case DMR_STATE::WAITING_LOGIN:  stage = "LOGIN";   break;
    case DMR_STATE::WAITING_AUTH:   stage = "AUTH";    break;
    case DMR_STATE::WAITING_CONFIG: stage = "CONFIG";  break;
    default:                        stage = "UNKNOWN"; break;
    }
    addLogMessage("[DMR] BrandMeister NAK at stage: " + stage);
    publishMqtt(mqttDmrTaskTopic.c_str(), ("{\"event\":\"nak\",\"stage\":\"" + stage + "\"}").c_str());

    dmrState = DMR_STATE::DISCONNECTED;
    return;
  }

  // --- RPTACK: Positive acknowledgment (drives state machine) ---
  if (len >= 10 && memcmp(packet, "RPTACK", 6) == 0)
  {
    switch (dmrState)
    {
    case DMR_STATE::WAITING_LOGIN:
    {
      // Save the 4-byte salt from the ACK
      memcpy(dmrSalt, packet + 6, 4);

      String saltHex = "[DMR] Login ACK, salt: ";
      for (int i = 0; i < 4; i++)
      {
        if (dmrSalt[i] < 0x10) saltHex += "0";
        saltHex += String(dmrSalt[i], HEX);
      }
      addLogMessage(saltHex);

      dmrState = DMR_STATE::WAITING_AUTH;
      sendDMRAuth(dmrSalt);
      break;
    }
    case DMR_STATE::WAITING_AUTH:
      addLogMessage("[DMR] Auth ACK received, sending config...");
      dmrState = DMR_STATE::WAITING_CONFIG;
      sendDMRConfig();
      break;

    case DMR_STATE::WAITING_CONFIG:
      dmrLoggedIn = true;
      dmrLoginStatus = "Connected";
      dmrState = DMR_STATE::CONNECTED;
      loginAttempts = 0;
      lastKeepalive = millis();
      addLogMessage("[DMR] Config ACK - CONNECTED to BrandMeister!");
      publishMqtt(mqttDmrTaskTopic.c_str(), "{\"event\":\"connected\"}");
      break;

    case DMR_STATE::DISCONNECTED:
      addLogMessage("[DMR] RPTACK received but DISCONNECTED - ignoring");
      break;

    default:
      addLogMessage("[DMR] RPTACK in unexpected state: " + String((int)dmrState));
      break;
    }
    return;
  }

  // --- MSTPONG: Keepalive response ---
  if (len >= 7 && memcmp(packet, "MSTPONG", 7) == 0)
  {
    // Keepalive acknowledged - no action needed
    return;
  }

  // --- MSTCL: Master closing connection ---
  if (len >= 5 && memcmp(packet, "MSTCL", 5) == 0)
  {
    addLogMessage("[DMR] Master closed connection (MSTCL)");
    publishMqtt(mqttDmrTaskTopic.c_str(), "{\"event\":\"master_closed\"}");
    dmrLoggedIn = false;
    dmrLoginStatus = "Disconnected by server";
    dmrState = DMR_STATE::DISCONNECTED;
    return;
  }

  // --- DMRD: Incoming DMR data from BrandMeister ---
  if (len >= 55 && memcmp(packet, "DMRD", 4) == 0)
  {
    // Forward raw DMRD packet to the remote node over ESP-NOW (sender mode).
    // The remote node runs the same firmware and processes it independently —
    // full callsign lookup, OLED, web UI, and modem TX all work without internet.
#if ESPNOW_SENDER
    if (espnowDmrEnabled) espnowSendDmrNetPacket(packet, (uint8_t)len);
#endif
    processDMRDPacket(packet, len);
    return;
  }
}

// ===== DMR Modem: Send DMR START/STOP command =====
void writeDMRStart(bool tx)
{
  uint8_t buffer[4];
  buffer[0] = MMDVM_FRAME_START;
  buffer[1] = 4;
  buffer[2] = CMD_DMR_START;
  buffer[3] = tx ? 0x01 : 0x00;

  MMDVM_SERIAL.write(buffer, 4);
  MMDVM_SERIAL.flush();

  addLogMessage(tx ? "[DMR] Modem TX START" : "[DMR] Modem TX STOP");
}

/*
 * TASK: DMR Protocol Handler
 * Priority: High - Real-time digital voice protocol
 *
 * Handles:
 * - BrandMeister network login state machine
 * - Keepalive packets
 * - Incoming network DMR data
 * - Incoming modem DMR frames
 */
void dmrTask(void *parameter)
{
  // Buffer for incoming modem serial frames
  uint8_t modemRxBuffer[512];
  int modemRxPtr = 0;
  addLogMessage("[DMR Task] Started - waiting for modem and network...");
  publishMqtt(mqttDmrTaskTopic.c_str(), "{\"event\":\"started\"}");

  // Wait for modem to be ready
  int modemWaitCount = 0;
  while (!mmdvmReady && modemWaitCount < 100)
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    modemWaitCount++;
  }

  if (!mmdvmReady)
  {
    addLogMessage("[DMR Task] ERROR: Modem not ready, task suspended");
    publishMqtt(mqttDmrTaskTopic.c_str(), "{\"event\":\"error\",\"reason\":\"modem_not_ready\"}");
    vTaskSuspend(NULL);
    return;
  }

  addLogMessage("[DMR Task] Modem firmware: " + modemFirmwareVersion);

  if (dmrServerEspNow)
  {
    // ESP-NOW relay mode: BrandMeister is not used.
    // Frames arrive via the ESP-NOW receive queue and are processed by
    // handleDMRNetwork() → processDMRDPacket() on every loop iteration.
    // WiFi driver must be up for ESP-NOW (handled by initEspNow()), but we
    // do not need a routed internet connection.
    addLogMessage("[DMR Task] ESP-NOW relay mode — BrandMeister disabled, receiving via ESP-NOW");
    publishMqtt(mqttDmrTaskTopic.c_str(), "{\"event\":\"espnow_relay_ready\"}");
    dmrLoginStatus = "ESP-NOW Relay";
  }
  else
  {
    // Wait for any network connection (WiFi OR Ethernet)
    while (WiFi.status() != WL_CONNECTED && !ethConnected)
    {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    addLogMessage("[DMR Task] Network connected - starting DMR network login...");
    publishMqtt(mqttDmrTaskTopic.c_str(), "{\"event\":\"network_ready\"}");
  }

  while (true)
  {
    unsigned long now = millis();

    // ===== 1. Handle incoming DMR network packets (UDP or ESP-NOW queue) =====
    handleDMRNetwork();

    // ===== 2. Login timeout and retry logic (BrandMeister only) =====
    if (!dmrServerEspNow && !dmrLoggedIn)
    {
      if (dmrState == DMR_STATE::WAITING_LOGIN ||
          dmrState == DMR_STATE::WAITING_AUTH ||
          dmrState == DMR_STATE::WAITING_CONFIG)
      {
        // Check for login timeout
        if (now - lastLoginAttempt >= DMR_LOGIN_TIMEOUT)
        {
          if (loginAttempts < DMR_LOGIN_MAX_RETRIES)
          {
            loginAttempts++;
            addLogMessage("[DMR] Login timeout - retrying (" + String(loginAttempts) + "/" + String(DMR_LOGIN_MAX_RETRIES) + ")");
            dmrLoginStatus = "Retrying (" + String(loginAttempts) + "/" + String(DMR_LOGIN_MAX_RETRIES) + ")";
            publishMqtt(mqttDmrTaskTopic.c_str(),
                        ("{\"event\":\"login_retry\",\"attempt\":" + String(loginAttempts) +
                         ",\"max\":" + String(DMR_LOGIN_MAX_RETRIES) + "}").c_str());
            connectToDMRNetwork();
          }
          else
          {
            addLogMessage("[DMR] Login failed after " + String(DMR_LOGIN_MAX_RETRIES) + " attempts");
            publishMqtt(mqttDmrTaskTopic.c_str(), "{\"event\":\"login_failed\",\"reason\":\"max_retries\"}");
            dmrLoginStatus = "Login Failed";
            dmrState = DMR_STATE::DISCONNECTED;
          }
        }
      }
      else if (dmrState == DMR_STATE::DISCONNECTED)
      {
        // Auto-reconnect: immediate on first boot, 30s delay after a disconnect
        static unsigned long lastReconnectAttempt = 0;
        static bool firstConnect = true;
        if (firstConnect || (now - lastReconnectAttempt >= 30000))
        {
          if (firstConnect)
            addLogMessage("[DMR] Connecting to BrandMeister...");
          else
            addLogMessage("[DMR] Auto-reconnecting to BrandMeister...");
          firstConnect = false;
          publishMqtt(mqttDmrTaskTopic.c_str(), "{\"event\":\"reconnecting\"}");
          loginAttempts = 0;
          connectToDMRNetwork();
          lastReconnectAttempt = now;
        }
      }
    }

    // ===== 3. Keepalive (BrandMeister only) =====
    if (!dmrServerEspNow && dmrLoggedIn && dmrState == DMR_STATE::CONNECTED)
    {
      if (now - lastKeepalive >= DMR_KEEPALIVE_INTERVAL)
      {
        sendDMRKeepalive();
        lastKeepalive = now;
      }
    }

    // ===== 4. TX / RX timeouts =====
    // RF RX: clear if no DMR data frame received from modem for 500ms
    if (dmrRfRxActive && (millis() - dmrRfRxLastFrameMs > 500))
    {
      dmrRfRxActive = false;
    }
    // Network layer: end session after 3s silence — stop modem TX here (once per call),
    // NOT on a per-frame timer. The modem fills mid-call gaps with idle frames
    // automatically via its own DMRTX state machine; calling writeDMRStart(false/true)
    // mid-call resets m_frameCount and causes a 1.2s startup silence every time.
    if (netRxActive && (millis() - netRxLastPktTime > NET_RX_IDLE_MS))
    {
      netRxActive = false;
      if (dmrTxActive) {
        writeDMRStart(false);
        dmrTxActive = false;
      }
      addLogMessage("[DMR] Net→RF ended: " + String(netRxSrcId) + "→" + String(netRxDstId));
      String json = "{\"event\":\"ended\""
                    ",\"src\":" + String(netRxSrcId) +
                    ",\"dst\":" + String(netRxDstId) +
                    ",\"callsign\":\"" + jsonStr(netRxCallsign) + "\"}";
      publishMqtt(mqttDmrTaskTopic.c_str(), json.c_str());
    }

    // ===== 5. Paced DMR TX to modem (skip if POCSAG owns modem) =====
    static unsigned long lastTxPaced = 0;
    if (dmrTxTail != dmrTxHead && !pocsagTxInProgress)
    {
      unsigned long nowPaced = millis();
      if (nowPaced - lastTxPaced >= 55) // ~55ms between frames = 34 bytes at 9600bps with some margin
      {
        DmrTxFrame &frame = dmrTxBuffer[dmrTxTail];
        if (frame.valid)
        {
          // Send DMR START once at beginning of transmission
          if (!dmrTxActive)
          {
            writeDMRStart(true);
            dmrTxActive = true;
          }
          sendMMDVMCommand(frame.cmd, frame.data, 34);
          frame.valid = false;
          dmrTxTail = (dmrTxTail + 1) % DMR_TX_BUFFER_SIZE;
          lastTxPaced = nowPaced;
        }
      }
    }

    // ===== 6. Process incoming modem serial frames =====
    // Skip reading while POCSAG owns the UART — consuming bytes here would
    // steal the ACK responses that waitForModemAck() is expecting.
    if (pocsagTxInProgress) {
      modemRxPtr = 0; // reset partial frame so we start clean when POCSAG is done
    } else {
      while (MMDVM_SERIAL.available())
      {
        uint8_t byte = MMDVM_SERIAL.read();
        if (modemRxPtr == 0 && byte != MMDVM_FRAME_START)
        {
          continue;
        }
        modemRxBuffer[modemRxPtr++] = byte;
        if (modemRxPtr >= 2)
        {
          uint8_t frameLength = modemRxBuffer[1];
          if (modemRxPtr >= frameLength && frameLength >= 3)
          {
            uint8_t cmd = modemRxBuffer[2];
            switch (cmd)
            {
              case CMD_DMR_DATA1:
                addLogMessage("[DMR] RF slot 1 data (" + String(frameLength - 3) + " bytes)");
                dmrRfRxActive = true;
                dmrRfRxLastFrameMs = millis();
                break;
              case CMD_DMR_DATA2:
                addLogMessage("[DMR] RF slot 2 data (" + String(frameLength - 3) + " bytes)");
                dmrRfRxActive = true;
                dmrRfRxLastFrameMs = millis();
                break;
            }
            modemRxPtr = 0;
          }
        }
        if (modemRxPtr >= sizeof(modemRxBuffer))
          modemRxPtr = 0;
      }
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

}

// ===== DMR User Info Lookup (callsign, name, location, etc.) =====
// Sequence: Cache -> SQLite -> RadioID.net API -> Cache

//#define DMR_USER_CACHE_SIZE 32
struct DmrUserCacheEntry {
    uint32_t dmrId;
    String userInfo;
    unsigned long timestamp;
};
static DmrUserCacheEntry dmrUserCache[DMR_USER_CACHE_SIZE];
static int dmrUserCacheIndex = 0;

String getCachedDmrUserInfo(uint32_t dmrId) {
    for (int i = 0; i < DMR_USER_CACHE_SIZE; i++) {
        if (dmrUserCache[i].dmrId == dmrId && dmrUserCache[i].userInfo.length() > 0) {
            return dmrUserCache[i].userInfo;
        }
    }
    return "";
}

void cacheDmrUserInfo(uint32_t dmrId, String userInfo) {
    dmrUserCache[dmrUserCacheIndex].dmrId = dmrId;
    dmrUserCache[dmrUserCacheIndex].userInfo = userInfo;
    dmrUserCache[dmrUserCacheIndex].timestamp = millis();
    dmrUserCacheIndex = (dmrUserCacheIndex + 1) % DMR_USER_CACHE_SIZE;
}

String lookupDmrUserInfo(uint32_t dmrId, String &source) {
    source = "not_found";
    if (dmrId == 0) return "";

    addLogMessage("[DMR-LOOKUP] Lookup for DMR ID: " + String(dmrId));

    // Step 1: Check cache
    String cached = getCachedDmrUserInfo(dmrId);
    if (cached.length() > 0) {
      addLogMessage("[DMR-LOOKUP] Found in cache: " + cached);
      source = "cache";
      return cached;
    }

    // Step 2: Try local SQLite database (if enabled)
    String userInfo = "";
    if (sdcardEnabled) {
        addLogMessage("[DMR-LOOKUP] Trying SQLite DB for DMR ID: " + String(dmrId));
        if (sdCardMounted && SD.exists(SDCARD_SQLITE_FILE)) {
            sqlite3 *db = NULL;
            String dbPath = "/sd" + String(SDCARD_SQLITE_FILE);
            int rc = sqlite3_open(dbPath.c_str(), &db);
            if (rc == SQLITE_OK) {
                // Use correct table (radioid), indexed field (RADIO_ID), and FIRST_NAME
                // The table radioid columns: RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY
                String sql = "SELECT CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE RADIO_ID = ? LIMIT 1;";
                #ifdef DEBUG_DMR_LOOKUP
                addLogMessage("[DMR-LOOKUP] SQLite query: " + sql);
                #endif
                sqlite3_stmt *stmt = nullptr;
                rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
                if (rc == SQLITE_OK) {
                    sqlite3_bind_int(stmt, 1, dmrId);
                    if (sqlite3_step(stmt) == SQLITE_ROW) {
                        String callsign = sqlite3_column_text(stmt, 0) ? String((const char*)sqlite3_column_text(stmt, 0)) : "";
                        String firstName = sqlite3_column_text(stmt, 1) ? String((const char*)sqlite3_column_text(stmt, 1)) : "";
                        String city = sqlite3_column_text(stmt, 2) ? String((const char*)sqlite3_column_text(stmt, 2)) : "";
                        String state = sqlite3_column_text(stmt, 3) ? String((const char*)sqlite3_column_text(stmt, 3)) : "";
                        String country = sqlite3_column_text(stmt, 4) ? String((const char*)sqlite3_column_text(stmt, 4)) : "";
                        addLogMessage("[DMR-LOOKUP] SQLite result: callsign=" + callsign + ", firstName=" + firstName + ", city=" + city + ", state=" + state + ", country=" + country);
                        if (callsign.length() > 0) {
                            userInfo = callsign;
                            if (firstName.length() > 0 || city.length() > 0 || state.length() > 0 || country.length() > 0) {
                                userInfo += "|" + firstName + "|" + city + "|" + state + "|" + country;
                            }
                            addLogMessage("[DMR-LOOKUP] Found in SQLite: " + userInfo);
                        }
                    } else {
                        addLogMessage("[DMR-LOOKUP] Not found in SQLite for DMR ID: " + String(dmrId));
                    }
                } else {
                    addLogMessage("[DMR-LOOKUP] SQLite prepare error for DMR ID: " + String(dmrId) + ", rc=" + String(rc));
                }
                sqlite3_finalize(stmt);
                sqlite3_close(db);
            } else {
                addLogMessage("[DMR-LOOKUP] SQLite open error for DMR ID: " + String(dmrId));
            }
        } else {
            addLogMessage("[DMR-LOOKUP] SQLite not available for DMR ID: " + String(dmrId));
        }
    }
    if (userInfo.length() > 0) {
      cacheDmrUserInfo(dmrId, userInfo);
      source = "database";
      return userInfo;
    }

    // Step 3: Fallback to RadioID.net API
    addLogMessage("[DMR-LOOKUP] Querying RadioID.net API for DMR ID: " + String(dmrId));
    // #define DMR_API_URL "https://radioid.net/api/dmr/user/?id="
    // #define DMR_API_TIMEOUT 3000
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClientSecure secureClient;
      secureClient.setInsecure(); // Skip cert verification — radioid.net HTTPS
      HTTPClient http;
      http.begin(secureClient, dmrApiUrl + String(dmrId));
      http.setTimeout(dmrApiTimeout);
      int httpCode = http.GET();
      if (httpCode == 200) {
        String payload = http.getString();
        String callsign = "";
        String name = "";
        String city = "";
        String state = "";
        String country = "";
        int csIndex = payload.indexOf("\"callsign\":\"");
        if (csIndex > 0) {
          csIndex += 12;
          int endIndex = payload.indexOf("\"", csIndex);
          if (endIndex > csIndex) {
            callsign = payload.substring(csIndex, endIndex);
          }
        }
        int nameIndex = payload.indexOf("\"name\":\"");
        if (nameIndex > 0) {
          nameIndex += 8;
          int endIndex = payload.indexOf("\"", nameIndex);
          if (endIndex > nameIndex) {
            name = payload.substring(nameIndex, endIndex);
            if (name == "null" || name.length() == 0) {
              int fnameIndex = payload.indexOf("\"fname\":\"");
              if (fnameIndex > 0) {
                fnameIndex += 9;
                int fendIndex = payload.indexOf("\"", fnameIndex);
                if (fendIndex > fnameIndex) {
                  name = payload.substring(fnameIndex, fendIndex);
                }
              }
            }
          }
        }
        int cityIndex = payload.indexOf("\"city\":\"");
        if (cityIndex > 0) {
          cityIndex += 8;
          int endIndex = payload.indexOf("\"", cityIndex);
          if (endIndex > cityIndex) {
            city = payload.substring(cityIndex, endIndex);
            if (city == "null") city = "";
          }
        }
        int stateIndex = payload.indexOf("\"state\":\"");
        if (stateIndex > 0) {
          stateIndex += 9;
          int endIndex = payload.indexOf("\"", stateIndex);
          if (endIndex > stateIndex) {
            state = payload.substring(stateIndex, endIndex);
            if (state == "null") state = "";
          }
        }
        int countryIndex = payload.indexOf("\"country\":\"");
        if (countryIndex > 0) {
          countryIndex += 11;
          int endIndex = payload.indexOf("\"", countryIndex);
          if (endIndex > countryIndex) {
            country = payload.substring(countryIndex, endIndex);
            if (country == "null") country = "";
          }
        }
        if (callsign.length() > 0) {
          userInfo = callsign;
          if (name.length() > 0 || city.length() > 0 || state.length() > 0 || country.length() > 0) {
            userInfo += "|" + name + "|" + city + "|" + state + "|" + country;
          }
          addLogMessage("[DMR-LOOKUP] Found in RadioID.net: " + userInfo);
        } else {
          addLogMessage("[DMR-LOOKUP] No result from RadioID.net for DMR ID: " + String(dmrId));
        }
      } else {
        addLogMessage("[DMR-LOOKUP] RadioID.net HTTP error: " + String(httpCode));
      }
      http.end();
    } else {
      addLogMessage("[DMR-LOOKUP] WiFi not connected, cannot query RadioID.net");
    }
    if (userInfo.length() > 0) {
      cacheDmrUserInfo(dmrId, userInfo);
      source = "online";
      return userInfo;
    }
    addLogMessage("[DMR-LOOKUP] No user info found for DMR ID: " + String(dmrId));
    return "";
}

// Usage example:
// String userInfo = lookupDmrUserInfo(dmrId);
// Format: "callsign|name|city|state|country"

// ===== Async DMR User Lookup Task =====
// Performs blocking SQLite / HTTP lookups off the DMR task so keepalives are not delayed.

static void parseDmrUserInfo(const String &userInfo,
                              String &callsign, String &name,
                              String &city, String &state, String &country) {
    callsign = name = city = state = country = "";
    int pipe1 = userInfo.indexOf('|');
    int pipe2 = (pipe1 >= 0) ? userInfo.indexOf('|', pipe1 + 1) : -1;
    int pipe3 = (pipe2 >= 0) ? userInfo.indexOf('|', pipe2 + 1) : -1;
    int pipe4 = (pipe3 >= 0) ? userInfo.indexOf('|', pipe3 + 1) : -1;
    if (pipe1 > 0) {
        callsign = userInfo.substring(0, pipe1);
        if (pipe2 > pipe1) name = userInfo.substring(pipe1 + 1, pipe2);
        else                name = userInfo.substring(pipe1 + 1);
        if (pipe3 > pipe2) city  = userInfo.substring(pipe2 + 1, pipe3);
        if (pipe4 > pipe3) state = userInfo.substring(pipe3 + 1, pipe4);
        if (pipe4 > 0)     country = userInfo.substring(pipe4 + 1);
    } else {
        callsign = userInfo;
    }
}

void dmrLookupTask(void *parameter) {
    uint32_t dmrId;
    while (true) {
        if (xQueueReceive(dmrLookupQueue, &dmrId, portMAX_DELAY)) {
            String source;
            String userInfo = lookupDmrUserInfo(dmrId, source);
            String callsign, name, city, state, country;
            if (userInfo.length() > 0) {
                parseDmrUserInfo(userInfo, callsign, name, city, state, country);
                // Update OLED display with resolved info
                setDmrTxUserInfo(callsign, String(dmrId), name, city, state, country);
                // Update stored callsign for the "ended" event
                if (dmrId == netRxSrcId) {
                    netRxCallsign = callsign;
                }
            }
            // Always publish user_info for the active transmission so subscribers
            // know the lookup is complete. found:false means ID is not in any database.
            if (dmrId == netRxSrcId) {
                String json = "{\"event\":\"user_info\""
                              ",\"src\":" + String(dmrId) +
                              ",\"found\":" + (userInfo.length() > 0 ? "true" : "false") +
                              ",\"source\":\"" + source + "\"" +
                              ",\"callsign\":\"" + jsonStr(callsign) + "\"" +
                              ",\"name\":\"" + jsonStr(name) + "\"" +
                              ",\"city\":\"" + jsonStr(city) + "\"" +
                              ",\"state\":\"" + jsonStr(state) + "\"" +
                              ",\"country\":\"" + jsonStr(country) + "\"}";
                publishMqtt(mqttDmrTaskTopic.c_str(), json.c_str());
            }
        }
    }
}

// ===== Talker Alias (TA) from Embedded LC — Self-contained port of MMDVMHost =====
// Pipeline: DMRD voice frame → EMB → QR decode → LCSS → 4-block LC assembly
//           → column deinterleave → Hamming(16,11,4) → CRC5 → FLCO → TA text

// QR(16,7,6) error pattern lookup table indexed by 8-bit syndrome.
// Source: DECODING_TABLE_1576 from MMDVMHost/QR1676.cpp (256 entries).
static const uint16_t TA_QR_TABLE[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x4020,
    0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x2081, 0x2080,
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0C00, 0x0016, 0x0C02,
    0x0018, 0x0120, 0x001A, 0x0122, 0x4102, 0x0124, 0x4100, 0x4101,
    0x0020, 0x0021, 0x0022, 0x4004, 0x0024, 0x4002, 0x4001, 0x4000,
    0x0028, 0x0110, 0x1800, 0x1801, 0x002C, 0x400A, 0x4009, 0x4008,
    0x0030, 0x0108, 0x0240, 0x0241, 0x0034, 0x4012, 0x4011, 0x4010,
    0x0101, 0x0100, 0x0103, 0x0102, 0x0105, 0x0104, 0x1401, 0x1400,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x4060,
    0x0048, 0x0049, 0x0301, 0x0300, 0x004C, 0x1600, 0x0305, 0x0304,
    0x0050, 0x0051, 0x0220, 0x0221, 0x3000, 0x4200, 0x3002, 0x4202,
    0x0058, 0x1082, 0x1081, 0x1080, 0x3008, 0x4208, 0x2820, 0x1084,
    0x0060, 0x0061, 0x0210, 0x0211, 0x0480, 0x0481, 0x4041, 0x4040,
    0x0068, 0x2402, 0x2401, 0x2400, 0x0488, 0x3100, 0x2810, 0x2404,
    0x0202, 0x0880, 0x0200, 0x0201, 0x0206, 0x0884, 0x0204, 0x0205,
    0x0141, 0x0140, 0x0208, 0x0209, 0x2802, 0x0144, 0x2800, 0x2801,
    0x0080, 0x0081, 0x0082, 0x0A00, 0x0084, 0x0085, 0x2009, 0x2008,
    0x0088, 0x0089, 0x2005, 0x2004, 0x2003, 0x2002, 0x2001, 0x2000,
    0x0090, 0x0091, 0x0092, 0x1048, 0x0602, 0x0C80, 0x0600, 0x0601,
    0x0098, 0x1042, 0x1041, 0x1040, 0x2013, 0x2012, 0x2011, 0x2010,
    0x00A0, 0x00A1, 0x00A2, 0x4084, 0x0440, 0x0441, 0x4081, 0x4080,
    0x6000, 0x1200, 0x6002, 0x1202, 0x6004, 0x2022, 0x2021, 0x2020,
    0x0841, 0x0840, 0x2104, 0x0842, 0x2102, 0x0844, 0x2100, 0x2101,
    0x0181, 0x0180, 0x0B00, 0x0182, 0x5040, 0x0184, 0x2108, 0x2030,
    0x00C0, 0x00C1, 0x4401, 0x4400, 0x0420, 0x0421, 0x0422, 0x4404,
    0x0900, 0x0901, 0x1011, 0x1010, 0x0904, 0x2042, 0x2041, 0x2040,
    0x0821, 0x0820, 0x1009, 0x1008, 0x4802, 0x0824, 0x4800, 0x4801,
    0x1003, 0x1002, 0x1001, 0x1000, 0x0501, 0x0500, 0x1005, 0x1004,
    0x0404, 0x0810, 0x1100, 0x1101, 0x0400, 0x0401, 0x0402, 0x0403,
    0x040C, 0x0818, 0x1108, 0x1030, 0x0408, 0x0409, 0x040A, 0x2060,
    0x0801, 0x0800, 0x0280, 0x0802, 0x0410, 0x0804, 0x0412, 0x0806,
    0x0809, 0x0808, 0x1021, 0x1020, 0x5000, 0x2200, 0x5002, 0x2202
};

// TA state machine — persists across frames within one transmission
enum TALCState : uint8_t { TA_LC_NONE, TA_LC_FIRST, TA_LC_SECOND, TA_LC_THIRD };
static TALCState taLCState      = TA_LC_NONE;
static bool      taRaw[128]     = {};  // 4 × 32 raw bits from the 4-block LCSS sequence
static uint8_t   taBuf[28]      = {};  // assembled TA service bytes (4 blocks × 7 bytes)
static bool      taHaveBlock[4] = {};  // which FLCO blocks (0=header, 1-3=continuations) received

static void taReset() {
    taLCState = TA_LC_NONE;
    memset(taRaw,       0, sizeof(taRaw));
    memset(taBuf,       0, sizeof(taBuf));
    memset(taHaveBlock, 0, sizeof(taHaveBlock));
}

// Extract LCSS from the EMB field of a 33-byte DMR voice frame using QR(16,7,6) decode.
// The EMB field is assembled from the upper nibbles of bytes 13-14 and 18-19.
// Returns LCSS bits [1:0] of the decoded 7-bit data word.
static uint8_t taGetLCSS(const uint8_t* f) {
    uint8_t emb[2];
    emb[0] = ((f[13] << 4) & 0xF0) | ((f[14] >> 4) & 0x0F);
    emb[1] = ((f[18] << 4) & 0xF0) | ((f[19] >> 4) & 0x0F);

    // Build 15-bit codeword: emb[0] in bits [14:7], emb[1]>>1 in bits [6:0]
    uint32_t code = ((uint32_t)emb[0] << 7) | (emb[1] >> 1);

    // Compute GF(2) polynomial remainder mod GENPOL = x^8+x^5+x^4+x^3+1 = 0x139
    uint32_t pat = code;
    uint32_t aux = 0x4000U;
    if (pat >= 0x100U) {
        while (pat & 0xFFFFFF00U) {
            while (!(aux & pat))
                aux >>= 1;
            pat ^= (aux >> 8) * 0x139U;
        }
    }

    // Apply error correction from lookup table, then extract decoded data bits
    code ^= TA_QR_TABLE[pat & 0xFF];
    uint8_t decoded = (uint8_t)(code >> 7);
    // decoded layout: [CC3 CC2 CC1 CC0 PI LCSS1 LCSS0 P7]
    return (decoded >> 1) & 0x03U;
}

// Hamming(16,11,4) single-bit error correction.
// d[0..10] = data bits, d[11..15] = parity bits.
// Returns false only for uncorrectable (2+ bit) errors.
static bool taHamming16114(bool* d) {
    bool c0 = d[0]^d[1]^d[2]^d[3]^d[5]^d[7]^d[8];
    bool c1 = d[1]^d[2]^d[3]^d[4]^d[6]^d[8]^d[9];
    bool c2 = d[2]^d[3]^d[4]^d[5]^d[7]^d[9]^d[10];
    bool c3 = d[0]^d[1]^d[2]^d[4]^d[6]^d[7]^d[10];
    bool c4 = d[0]^d[2]^d[5]^d[6]^d[8]^d[9]^d[10];
    uint8_t n = 0;
    n |= (c0 != d[11]) ? 0x01 : 0;
    n |= (c1 != d[12]) ? 0x02 : 0;
    n |= (c2 != d[13]) ? 0x04 : 0;
    n |= (c3 != d[14]) ? 0x08 : 0;
    n |= (c4 != d[15]) ? 0x10 : 0;
    switch (n) {
        case 0x00: return true;
        case 0x01: d[11]=!d[11]; return true;
        case 0x02: d[12]=!d[12]; return true;
        case 0x04: d[13]=!d[13]; return true;
        case 0x08: d[14]=!d[14]; return true;
        case 0x10: d[15]=!d[15]; return true;
        case 0x19: d[0] =!d[0];  return true;
        case 0x0B: d[1] =!d[1];  return true;
        case 0x1F: d[2] =!d[2];  return true;
        case 0x07: d[3] =!d[3];  return true;
        case 0x0E: d[4] =!d[4];  return true;
        case 0x15: d[5] =!d[5];  return true;
        case 0x1A: d[6] =!d[6];  return true;
        case 0x0D: d[7] =!d[7];  return true;
        case 0x13: d[8] =!d[8];  return true;
        case 0x16: d[9] =!d[9];  return true;
        case 0x1C: d[10]=!d[10]; return true;
        default:   return false;
    }
}

// Decode the 128-bit raw block (taRaw) into 9 LC bytes.
// Steps: column deinterleave → Hamming(16,11,4) × 7 rows → column parity → CRC5.
// Returns FLCO (lower 6 bits of lcOut[0]), or 0xFF on any error.
static uint8_t taDecodeLC(uint8_t lcOut[9]) {
    // Column deinterleave: taRaw was filled column-by-column, unpack row-by-row
    bool data[128];
    uint32_t b = 0;
    for (uint32_t a = 0; a < 128; a++) {
        data[b] = taRaw[a];
        b += 16;
        if (b > 127) b -= 127;
    }

    // Hamming(16,11,4) check rows 0-6 (16 bits each); row 7 (bits 112-127) is parity
    for (uint32_t a = 0; a < 112; a += 16) {
        if (!taHamming16114(data + a))
            return 0xFF;
    }

    // Column parity: XOR of rows 0-7 for each of 16 columns must be zero
    for (uint32_t a = 0; a < 16; a++) {
        bool p = data[a]^data[a+16]^data[a+32]^data[a+48]^
                 data[a+64]^data[a+80]^data[a+96]^data[a+112];
        if (p) return 0xFF;
    }

    // Extract 72 payload bits (11 bits from each of the 7 data rows, columns 0-10)
    bool bits72[72];
    b = 0;
    for (uint32_t a =  0; a < 11; a++, b++) bits72[b] = data[a];
    for (uint32_t a = 16; a < 27; a++, b++) bits72[b] = data[a];
    for (uint32_t a = 32; a < 42; a++, b++) bits72[b] = data[a];
    for (uint32_t a = 48; a < 58; a++, b++) bits72[b] = data[a];
    for (uint32_t a = 64; a < 74; a++, b++) bits72[b] = data[a];
    for (uint32_t a = 80; a < 90; a++, b++) bits72[b] = data[a];
    for (uint32_t a = 96; a <106; a++, b++) bits72[b] = data[a];

    // 5-bit CRC is stored in column 10 of rows 2-6 (bits 42, 58, 74, 90, 106)
    uint32_t crc = 0;
    if (data[42])  crc += 16;
    if (data[58])  crc +=  8;
    if (data[74])  crc +=  4;
    if (data[90])  crc +=  2;
    if (data[106]) crc +=  1;

    // CRC5 = sum of 9 decoded bytes mod 31
    uint16_t total = 0;
    for (uint32_t i = 0; i < 72; i += 8) {
        uint8_t c = 0;
        for (int j = 0; j < 8; j++)
            c = (c << 1) | (bits72[i+j] ? 1 : 0);
        total += c;
        lcOut[i / 8] = c;
    }
    if ((total % 31) != crc) return 0xFF;

    return lcOut[0] & 0x3F;  // FLCO = lower 6 bits of byte 0
}

// Accumulate one embedded LC block from a voice frame.
// Advances the LCSS state machine (NONE → FIRST → SECOND → THIRD → NONE+decode).
// frame33: 33-byte voice frame. lcss: 0-3 from taGetLCSS().
// On 4-block completion, decodes and returns FLCO; otherwise returns 0xFF.
static uint8_t taAccumulate(const uint8_t* frame33, uint8_t lcss, uint8_t lcOut[9]) {
    // The embedded LC occupies: lower nibble of byte 14, all of bytes 15-17,
    // upper nibble of byte 18 → 32 bits per LCSS block (skip first 4 bits).
    bool raw[40];
    for (int b = 0; b < 8; b++) raw[ 0+b] = (frame33[14] >> (7-b)) & 1;
    for (int b = 0; b < 8; b++) raw[ 8+b] = (frame33[15] >> (7-b)) & 1;
    for (int b = 0; b < 8; b++) raw[16+b] = (frame33[16] >> (7-b)) & 1;
    for (int b = 0; b < 8; b++) raw[24+b] = (frame33[17] >> (7-b)) & 1;
    for (int b = 0; b < 8; b++) raw[32+b] = (frame33[18] >> (7-b)) & 1;
    // raw[4..35] are the 32 LC bits (raw[0..3] overlap with EMB upper nibble)

    if (lcss == 1) {
        for (int a = 0; a < 32; a++) taRaw[a] = raw[a+4];
        taLCState = TA_LC_FIRST;
        return 0xFF;
    }
    if (lcss == 3 && taLCState == TA_LC_FIRST) {
        for (int a = 0; a < 32; a++) taRaw[32+a] = raw[a+4];
        taLCState = TA_LC_SECOND;
        return 0xFF;
    }
    if (lcss == 3 && taLCState == TA_LC_SECOND) {
        for (int a = 0; a < 32; a++) taRaw[64+a] = raw[a+4];
        taLCState = TA_LC_THIRD;
        return 0xFF;
    }
    if (lcss == 2 && taLCState == TA_LC_THIRD) {
        for (int a = 0; a < 32; a++) taRaw[96+a] = raw[a+4];
        taLCState = TA_LC_NONE;
        return taDecodeLC(lcOut);
    }
    return 0xFF;
}

// Decode TA text from the accumulated taBuf (4 blocks × 7 service bytes).
// taBuf[0]: format (bits 7-6) + size in chars (bits 5-1).
// Formats: 0=7-bit packed, 1=ISO-8859-1, 2=UTF-8, 3=UTF-16.
static String taDecodeText() {
    uint8_t fmt  = (taBuf[0] >> 6) & 0x03;
    uint8_t size = (taBuf[0] >> 1) & 0x1F;
    if (size == 0 || size > 27) return "";

    String out;
    if (fmt == 0) {
        // 7-bit packed: stream of 7-bit characters packed MSB-first into bytes
        uint32_t t1 = 0, t2 = 0;
        uint8_t  c  = 0;
        for (uint32_t i = 0; i < 32 && t2 < size; i++) {
            for (int j = 7; j >= 0; j--) {
                c = (c << 1) | ((taBuf[i] >> j) & 1);
                if (++t1 == 7) {
                    if (i > 0) {
                        char ch = c & 0x7F;
                        if (ch >= 0x20) out += ch;
                        t2++;
                    }
                    t1 = 0; c = 0;
                }
            }
        }
    } else if (fmt == 1 || fmt == 2) {
        // ISO-8859-1 or UTF-8: direct byte copy starting at taBuf[1]
        for (uint32_t i = 1; i <= size && i < 28; i++) {
            char ch = (char)taBuf[i];
            if (ch == 0) break;
            out += ch;
        }
    } else {
        // UTF-16: simplified — take low byte of each 2-byte code unit if high byte is 0
        uint32_t t2 = 0;
        for (uint32_t i = 0; i < 15 && t2 < size; i++) {
            out += (taBuf[2*i+1] == 0) ? (char)taBuf[2*i+2] : '?';
            t2++;
        }
    }
    return out;
}

// Main TA extraction entry point. Call once per incoming DMRD voice frame.
// frame33: 33-byte voice frame (packet[20..52]).
// isNewTx: true for the first frame of a new transmission (resets state).
// Returns TA text when newly decoded, empty string while accumulating or on error.
static String tryExtractTA(const uint8_t* frame33, bool isNewTx) {
    if (isNewTx) taReset();

    uint8_t lcss = taGetLCSS(frame33);
    uint8_t lcBytes[9] = {};
    uint8_t flco = taAccumulate(frame33, lcss, lcBytes);
    if (flco == 0xFF) return "";

    // FLCO 4 = TA header (first block), 5/6/7 = continuation blocks
    if (flco < 4 || flco > 7) return "";

    uint32_t blockId = flco - 4;
    if (!taHaveBlock[blockId]) {
        // Copy 7 service bytes (lcBytes[2..8], skipping FLCO byte and FID byte)
        memcpy(taBuf + blockId * 7, lcBytes + 2, 7);
        taHaveBlock[blockId] = true;
    }

    // Need at least block 0 (header) to know format and declared size
    if (!taHaveBlock[0]) return "";

    return taDecodeText();
}

void initDmrLookupTask() {
    dmrLookupQueue = xQueueCreate(4, sizeof(uint32_t));
    BaseType_t result = xTaskCreatePinnedToCore(
        dmrLookupTask,
        "DMR Database",
        8192,
        NULL,
        1,    // low priority — blocking is fine here
        NULL,
        0     // core 0 — network I/O
    );
    if (result != pdPASS)
        log_e("[DMR Lookup] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());
}
