/*
 * ESP-NOW Transmitter
 *
 * Connects to BrandMeister (DMR Homebrew UDP) and/or DAPNET (POCSAG TCP) and
 * forwards received frames over ESP-NOW to hotspot receivers running the main
 * ESP32 MMDVM firmware with dmrServerEspNow / pocsagServerEspNow enabled.
 *
 * Packet format matches system_espnow.h in the main firmware exactly:
 *   EspNowDmrNetPacket  (type 0x10) — raw DMRD Homebrew frame
 *   EspNowPocsagPacket  (type 0x11) — POCSAG page (RIC + functional + text)
 *
 * Configuration is stored in NVS (namespace "txmitter") and edited via the
 * embedded web interface at http://<device-ip>/
 *
 * Architecture
 * ───────────────────────────────────────────────────────────────────────────
 *  core 0: bmTask()     — BrandMeister UDP state machine + ESP-NOW forward
 *  core 0: dapnetTask() — DAPNET TCP client + ESP-NOW forward
 *  core 1: loop()       — WebServer.handleClient() (Arduino default core)
 *
 * Receiver side (hotspot firmware, not this sketch):
 *   Set dmrServerEspNow  = true  →  hotspot ignores BrandMeister, receives
 *                                    DMRD frames from ESP-NOW and drives modem
 *   Set pocsagServerEspNow = true →  hotspot ignores DAPNET, receives POCSAG
 *                                    pages from ESP-NOW and drives modem
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <mbedtls/md.h>

#include "config.h"
#include "styles.h"
#include "bm_servers.h"

// Global WebServer instance
WebServer server(80);

// Task handles for FreeRTOS tasks
TaskHandle_t bmTaskHandle = nullptr;
TaskHandle_t dapnetTaskHandle = nullptr;





// ─── Circular log buffer ─────────────────────────────────────────────────
static String logLines[LOG_LINES];
static int logHead = 0;
static int logCount = 0;
static portMUX_TYPE logMux = portMUX_INITIALIZER_UNLOCKED;

// LOGGING
void addLog(const String& msg) {
  Serial.println(msg); // Log the message to the serial console
  portENTER_CRITICAL(&logMux); // Enter critical section to protect shared resources
  logLines[logHead] = msg; // Store the log message in the circular buffer
  logHead = (logHead + 1) % LOG_LINES; // Update the head index
  if (logCount < LOG_LINES) logCount++; // Increment log count if not full
  portEXIT_CRITICAL(&logMux); // Exit critical section
}

struct __attribute__((packed)) EspNowDmrNetPacket {
  uint8_t type;      // ESPNOW_TYPE_DMR_NET
  uint8_t len;       // valid bytes in data[]
  uint8_t data[60];  // raw DMRD Homebrew bytes
};

struct __attribute__((packed)) EspNowPocsagPacket {
  uint8_t type;  // ESPNOW_TYPE_POCSAG
  uint32_t ric;
  uint8_t functional;
  char message[POCSAG_MSG_MAX + 1];
};

// ─── BrandMeister state machine ──────────────────────────────────────────
enum class BmState { DISCONNECTED,
                     WAITING_LOGIN,
                     WAITING_AUTH,
                     WAITING_CONFIG,
                     CONNECTED };

// ─── Settings ────────────────────────────────────────────────────────────

Preferences prefs;
// BrandMeister config fields (must be global)
String bmDescription = DEF_BM_DESCRIPTION;
String bmUrl = DEF_BM_URL;
String bmFirmwareVersion = DEF_BM_FIRMWARE_VERSION;

bool bmEnabled = DEF_BM_ENABLED;
bool bmDebug = DEF_BM_DEBUG;
String bmServer = DEF_BM_SERVER;
uint16_t bmPort = DEF_BM_PORT;
uint32_t bmDmrId = DEF_BM_DMR_ID;
String bmPassword = DEF_BM_PASSWORD;
String bmCallsign = DEF_BM_CALLSIGN;
uint8_t bmSsid = DEF_BM_SSID;
String bmLocation = DEF_BM_LOCATION;
String bmLatitude = DEF_BM_LATITUDE;
String bmLongitude = DEF_BM_LONGITUDE;
int bmHeight = DEF_BM_HEIGHT;
uint32_t bmRxFreq = DEF_BM_RX_FREQ;
uint32_t bmTxFreq = DEF_BM_TX_FREQ;

bool dapnetEnabled = DEF_DAPNET_ENABLED;
String dapnetServer = DEF_DAPNET_SERVER;
uint16_t dapnetPort = DEF_DAPNET_PORT;
String dapnetCallsign = DEF_DAPNET_CALLSIGN;  // empty = bmCallsign
String dapnetAuthKey = DEF_DAPNET_AUTHKEY;
bool dapnetDebug = DEF_DAPNET_DEBUG;

String espnowMacs = DEF_ESPNOW_MACS;

// ─── Runtime state ───────────────────────────────────────────────────────
volatile bool bmLoggedIn = false;
volatile bool dapnetLoggedIn = false;
String bmStatus = "Disabled";
String dapnetStatus = "Disabled";

// ─── DMRD Transmission Info for UI Card ───────────────────────────────
struct DmrdTxInfo {
  uint32_t srcId = 0;
  uint32_t dstId = 0;
  uint8_t slot = 0;
  bool isGroup = false;
  bool active = false;
  unsigned long lastUpdate = 0;
};
static DmrdTxInfo dmrdTxInfo;

static uint8_t espnowPeerMacs[ESPNOW_MAX_PEERS][6];
static int espnowPeerCount = 0;
static bool espnowReady = false;

// ─── DAPNET recent pages history ─────────────────────────────────────────
#define DAPNET_PAGE_HISTORY 3
struct DapnetPageEntry {
  uint32_t ric;
  uint8_t func;
  char msg[100];
  unsigned long rxMs;
};
static DapnetPageEntry dapnetPageHistory[DAPNET_PAGE_HISTORY];
static int dapnetPageHead = 0;
static int dapnetPageCount = 0;
static portMUX_TYPE dapnetPageMux = portMUX_INITIALIZER_UNLOCKED;




static String getLogsJson() {
  portENTER_CRITICAL(&logMux);
  String j = "[";
  int start = (logCount < LOG_LINES) ? 0 : logHead;
  for (int i = 0; i < logCount; i++) {
    int idx = (start + i) % LOG_LINES;
    if (i > 0) j += ",";
    String s = logLines[idx];
    s.replace("\\", "\\\\");
    s.replace("\"", "\\\"");
    j += "\"" + s + "\"";
  }
  j += "]";
  portEXIT_CRITICAL(&logMux);
  return j;
}


static void addDapnetPage(uint32_t ric, uint8_t func, const String& msg) {
  portENTER_CRITICAL(&dapnetPageMux);
  DapnetPageEntry& e = dapnetPageHistory[dapnetPageHead];
  e.ric = ric;
  e.func = func;
  msg.toCharArray(e.msg, sizeof(e.msg));
  e.rxMs = millis();
  dapnetPageHead = (dapnetPageHead + 1) % DAPNET_PAGE_HISTORY;
  if (dapnetPageCount < DAPNET_PAGE_HISTORY) dapnetPageCount++;
  portEXIT_CRITICAL(&dapnetPageMux);
}

// ══════════════════════════════════════════════════════════════════════════
// NVS SETTINGS
// ══════════════════════════════════════════════════════════════════════════

static String nvsGet(const char* key, const char* def) {
  String v = prefs.getString(key, def);
  return v.isEmpty() ? String(def) : v;
}

void loadSettings() {
  prefs.begin("txmitter", false);

  bmEnabled = prefs.getBool("bm_en", DEF_BM_ENABLED);
  bmServer = nvsGet("bm_server", DEF_BM_SERVER);
  bmPort = prefs.getUShort("bm_port", DEF_BM_PORT);
  bmDmrId = prefs.getUInt("bm_id", DEF_BM_DMR_ID);
  bmPassword = nvsGet("bm_pass", DEF_BM_PASSWORD);
  bmCallsign = nvsGet("bm_cs", DEF_BM_CALLSIGN);
  bmSsid = (uint8_t)prefs.getUChar("bm_ssid", DEF_BM_SSID);
  bmLocation = nvsGet("bm_location", DEF_BM_LOCATION);
  bmLatitude = nvsGet("bm_lat", DEF_BM_LATITUDE);
  bmLongitude = nvsGet("bm_lon", DEF_BM_LONGITUDE);
  bmHeight = (int)prefs.getInt("bm_height", DEF_BM_HEIGHT);
  bmRxFreq = prefs.getUInt("bm_rxf", DEF_BM_RX_FREQ);
  bmTxFreq = prefs.getUInt("bm_txf", DEF_BM_TX_FREQ);
  bmDescription = nvsGet("bm_desc", DEF_BM_DESCRIPTION);
  bmUrl = nvsGet("bm_url", DEF_BM_URL);
  bmFirmwareVersion = nvsGet("bm_fwver", DEF_BM_FIRMWARE_VERSION);
  bmDebug = prefs.getBool("bm_debug", DEF_BM_DEBUG);

  dapnetEnabled = prefs.getBool("dp_en", DEF_DAPNET_ENABLED);
  dapnetServer = nvsGet("dp_server", DEF_DAPNET_SERVER);
  dapnetPort = prefs.getUShort("dp_port", DEF_DAPNET_PORT);
  dapnetCallsign = prefs.getString("dp_cs", DEF_DAPNET_CALLSIGN);
  dapnetAuthKey = prefs.getString("dp_key", DEF_DAPNET_AUTHKEY);
  dapnetDebug = prefs.getBool("dp_debug", DEF_DAPNET_DEBUG);

  espnowMacs = prefs.getString("en_macs", DEF_ESPNOW_MACS);

  prefs.end();
  addLog("[Settings] Loaded from NVS");
}

void saveSettings() {
  prefs.begin("txmitter", false);

  prefs.putBool("bm_en", bmEnabled);
  prefs.putString("bm_server", bmServer);
  prefs.putUShort("bm_port", bmPort);
  prefs.putUInt("bm_id", bmDmrId);
  prefs.putString("bm_pass", bmPassword);
  prefs.putString("bm_cs", bmCallsign);
  prefs.putUChar("bm_ssid", bmSsid);
  prefs.putString("bm_location", bmLocation);
  prefs.putString("bm_lat", bmLatitude);
  prefs.putString("bm_lon", bmLongitude);
  prefs.putInt("bm_height", bmHeight);
  prefs.putUInt("bm_rxf", bmRxFreq);
  prefs.putUInt("bm_txf", bmTxFreq);
  prefs.putString("bm_desc", bmDescription);
  prefs.putString("bm_url", bmUrl);
  prefs.putString("bm_fwver", bmFirmwareVersion);
  prefs.putBool("bm_debug", bmDebug);

  prefs.putBool("dp_en", dapnetEnabled);
  prefs.putString("dp_server", dapnetServer);
  prefs.putUShort("dp_port", dapnetPort);
  prefs.putString("dp_cs", dapnetCallsign);
  prefs.putString("dp_key", dapnetAuthKey);
  prefs.putBool("dp_debug", dapnetDebug);

  prefs.putString("en_macs", espnowMacs);
  prefs.putBool("initialized", true);

  prefs.end();
  addLog("[Settings] Saved to NVS");
}


// ══════════════════════════════════════════════════════════════════════════
// MAC HELPERS
// ══════════════════════════════════════════════════════════════════════════

static bool parseMac(const String& str, uint8_t mac[6]) {
  unsigned int b[6] = {};
  if (sscanf(str.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X",
             &b[0], &b[1], &b[2], &b[3], &b[4], &b[5])
      != 6) return false;
  for (int i = 0; i < 6; i++) mac[i] = (uint8_t)b[i];
  return true;
}

static int parseMacList(const String& csv, uint8_t out[][6], int maxOut) {
  int count = 0;
  int start = 0;
  String s = csv;
  s.trim();
  while (count < maxOut) {
    int comma = s.indexOf(',', start);
    String entry = (comma < 0) ? s.substring(start) : s.substring(start, comma);
    entry.trim();
    entry.toUpperCase();
    if (entry.length() > 0 && parseMac(entry, out[count])) count++;
    if (comma < 0) break;
    start = comma + 1;
  }
  return count;
}


// ══════════════════════════════════════════════════════════════════════════
// ESP-NOW
// ══════════════════════════════════════════════════════════════════════════

static void onEspNowSend(const wifi_tx_info_t* info, esp_now_send_status_t status) {
  // No ACK is normal for peers with locally-administered MACs or channel mismatch.
  // Suppress to avoid log spam — delivery is confirmed by the receiver side.
  (void)info;
  (void)status;
}

void initEspNow() {
  espnowPeerCount = parseMacList(espnowMacs, espnowPeerMacs, ESPNOW_MAX_PEERS);
  if (espnowPeerCount == 0) {
    addLog("[ESP-NOW] No peers configured");
    return;
  }

  // Wait for WiFi driver to be ready
  uint8_t tmp[6];
  unsigned long t = millis();
  while (esp_wifi_get_mac(WIFI_IF_STA, tmp) != ESP_OK && millis() - t < 10000) delay(100);

  if (esp_now_init() != ESP_OK) {
    addLog("[ESP-NOW] Init FAILED");
    return;
  }
  esp_now_register_send_cb(onEspNowSend);

  for (int i = 0; i < espnowPeerCount; i++) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, espnowPeerMacs[i], 6);
    peer.channel = 0;
    peer.encrypt = false;
    if (esp_now_add_peer(&peer) == ESP_OK) {
      char mac[18];
      snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
               espnowPeerMacs[i][0], espnowPeerMacs[i][1], espnowPeerMacs[i][2],
               espnowPeerMacs[i][3], espnowPeerMacs[i][4], espnowPeerMacs[i][5]);
      addLog("[ESP-NOW] Added peer: " + String(mac));
    }
  }
  espnowReady = true;
  addLog("[ESP-NOW] Ready (" + String(espnowPeerCount) + " peer(s))");
}

static void espnowSendDmr(const uint8_t* dmrd, uint8_t len) {
  if (!espnowReady) return;
  EspNowDmrNetPacket pkt = {};
  pkt.type = ESPNOW_TYPE_DMR_NET;
  pkt.len = len;
  memcpy(pkt.data, dmrd, len < 60 ? len : 60);
  for (int i = 0; i < espnowPeerCount; i++)
    esp_now_send(espnowPeerMacs[i], (uint8_t*)&pkt, sizeof(pkt));
}

static void espnowSendPocsag(uint32_t ric, uint8_t functional, const char* msg) {
  if (!espnowReady) return;
  EspNowPocsagPacket pkt = {};
  pkt.type = ESPNOW_TYPE_POCSAG;
  pkt.ric = ric;
  pkt.functional = functional;
  strncpy(pkt.message, msg, POCSAG_MSG_MAX);
  pkt.message[POCSAG_MSG_MAX] = '\0';
  for (int i = 0; i < espnowPeerCount; i++)
    esp_now_send(espnowPeerMacs[i], (uint8_t*)&pkt, sizeof(pkt));
}


// ══════════════════════════════════════════════════════════════════════════
// BRANDMEISTER DMR TASK
//
// Protocol (UDP, default port 62031):
//   Client → "RPTL" + ID(4)               [WAITING_LOGIN]
//   Server → "RPTACK" + salt(4)
//   Client → "RPTK" + ID(4) + SHA256(salt+pass)(32)  [WAITING_AUTH]
//   Server → "RPTACK"
//   Client → "RPTC" + ID(4) + config(294) [WAITING_CONFIG]
//   Server → "RPTACK"                     [CONNECTED]
//   Client → "RPTPING" + ID(4) every 5 s
//   Server → "MSTPONG"
//   Server → "DMRD" frames (forwarded via ESP-NOW)
// ══════════════════════════════════════════════════════════════════════════

static WiFiUDP dmrUdp;
static BmState bmState = BmState::DISCONNECTED;
static uint8_t dmrSalt[4];

// Apply SSID to base DMR ID, matching main firmware getDmrIdToSend()
static uint32_t bmGetId() {
  return (bmSsid > 0) ? bmDmrId * 100 + bmSsid : bmDmrId;
}

static void bmSendRptl() {
  uint32_t id = bmGetId();
  uint8_t p[8];
  memcpy(p, "RPTL", 4);
  p[4] = (id >> 24) & 0xFF;
  p[5] = (id >> 16) & 0xFF;
  p[6] = (id >> 8) & 0xFF;
  p[7] = id & 0xFF;
  dmrUdp.beginPacket(bmServer.c_str(), bmPort);
  dmrUdp.write(p, 8);
  dmrUdp.endPacket();
}

static void bmSendRptk(const uint8_t* salt) {
  // SHA256(salt[4] || password_bytes)
  size_t passLen = bmPassword.length();
  size_t inputLen = 4 + passLen;
  uint8_t* input = new uint8_t[inputLen];
  memcpy(input, salt, 4);
  memcpy(input + 4, bmPassword.c_str(), passLen);

  uint8_t hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, input, inputLen);
  mbedtls_md_finish(&ctx, hash);
  mbedtls_md_free(&ctx);
  delete[] input;

  // RPTK: "RPTK"(4) + ID(4) + hash(32) = 40 bytes
  uint32_t id = bmGetId();
  uint8_t p[40];
  memcpy(p, "RPTK", 4);
  p[4] = (id >> 24) & 0xFF;
  p[5] = (id >> 16) & 0xFF;
  p[6] = (id >> 8) & 0xFF;
  p[7] = id & 0xFF;
  memcpy(p + 8, hash, 32);
  dmrUdp.beginPacket(bmServer.c_str(), bmPort);
  dmrUdp.write(p, 40);
  dmrUdp.endPacket();
}

static void bmSendRptc() {
  // Build 294-byte config string per MMDVMHost writeConfig() format:
  // "%-8.8s %09u %09u %02u %02u %8.8s %9.9s %03d %-20.20s %-19.19s %c %-124.124s %-40.40s %-40.40s"
  // (no spaces — shown above for readability only)
  String cs = bmCallsign;
  cs.toUpperCase();
  // Use bmDescription, bmUrl, bmFirmwareVersion for main firmware compatibility
  char lat[20], lon[20];
  snprintf(lat, sizeof(lat), "%08f", atof(bmLatitude.c_str()));
  snprintf(lon, sizeof(lon), "%09f", atof(bmLongitude.c_str()));
  int h = bmHeight;
  if (h > 999) h = 999;

  char cfg[295];
  memset(cfg, 0, sizeof(cfg));
  snprintf(cfg, sizeof(cfg),
           "%-8.8s%09u%09u%02u%02u%8.8s%9.9s%03d%-20.20s%-19.19s%c%-124.124s%-40.40s%-40.40s",
           cs.c_str(),                // Callsign  (8)
           bmRxFreq, bmTxFreq,        // RX / TX freq (9+9)
           99u,                       // Power (2) — match main firmware default
           1u,                        // Color code 1  (2)
           lat,                       // Latitude      (8)
           lon,                       // Longitude     (9)
           h,                         // Height m      (3)
           bmLocation.c_str(),        // Location      (20)
           bmDescription.c_str(),     // Description   (19)
           '4',                       // Slots '4'=simplex (1)
           bmUrl.c_str(),             // URL           (124)
           bmFirmwareVersion.c_str(), // Version       (40)
           "MMDVM_MMDVM_HS");         // Software      (40)

  // RPTC: "RPTC"(4) + ID(4) + config(294) = 302 bytes
  uint32_t id = bmGetId();

  if (bmDebug) {
    // Dump config fields exactly as sent — compare against main firmware RPTC log
    addLog("[BM] DBG RPTC cs='" + String(cs) + "' id=" + String(id));
    addLog("[BM] DBG RPTC rxf=" + String(bmRxFreq) + " txf=" + String(bmTxFreq) + " pwr=99 cc=1");
    addLog("[BM] DBG RPTC lat='" + String(lat) + "' lon='" + String(lon) + "' h=" + String(h));
    // Print first 30 chars of cfg (callsign+freq+pwr+cc+lat+lon partial)
    char preview[31];
    memcpy(preview, cfg, 30);
    preview[30] = '\0';
    addLog("[BM] DBG RPTC cfg[0..29]: '" + String(preview) + "'");
  }
  uint8_t p[302];
  memset(p, 0, sizeof(p));
  memcpy(p, "RPTC", 4);
  p[4] = (id >> 24) & 0xFF;
  p[5] = (id >> 16) & 0xFF;
  p[6] = (id >> 8) & 0xFF;
  p[7] = id & 0xFF;
  memcpy(p + 8, cfg, 294);
  dmrUdp.beginPacket(bmServer.c_str(), bmPort);
  dmrUdp.write(p, 302);
  dmrUdp.endPacket();
}

static void bmSendKeepalive() {
  // RPTPING: "RPTPING"(7) + ID(4) = 11 bytes
  uint32_t id = bmGetId();
  uint8_t p[11];
  memcpy(p, "RPTPING", 7);
  p[7] = (id >> 24) & 0xFF;
  p[8] = (id >> 16) & 0xFF;
  p[9] = (id >> 8) & 0xFF;
  p[10] = id & 0xFF;
  dmrUdp.beginPacket(bmServer.c_str(), bmPort);
  dmrUdp.write(p, 11);
  dmrUdp.endPacket();
}

void bmTask(void* param) {
  const unsigned long KEEPALIVE_MS = 5000;
  const unsigned long LOGIN_TO_MS = 15000;
  const unsigned long RECONNECT_MS = 30000;

  addLog("[BM] Task started");
  for (;;) {
    if (!bmEnabled) {
      bmStatus = "Disabled";
      vTaskDelay(pdMS_TO_TICKS(5000));
      continue;
    }

    while (WiFi.status() != WL_CONNECTED) {
      bmStatus = "Waiting for WiFi";
      vTaskDelay(pdMS_TO_TICKS(2000));
    }

    dmrUdp.begin(0);
    bmState = BmState::DISCONNECTED;
    bmLoggedIn = false;
    bmStatus = "Connecting...";
    uint32_t effectiveId = bmGetId();
    addLog("[BM] Connecting to " + bmServer + ":" + String(bmPort) + " id=" + String(effectiveId));
    if (bmDebug) {
      String passHint = bmPassword.length() > 0
                          ? String(bmPassword[0]) + String("***") + bmPassword[bmPassword.length() - 1]
                          : "(empty)";
      addLog("[BM] DBG callsign='" + bmCallsign + "' ssid=" + String(bmSsid) + " effectiveId=" + String(effectiveId) + " pass=" + passHint);
      addLog("[BM] DBG lat=" + bmLatitude + " lon=" + bmLongitude + " h=" + String(bmHeight) + " loc='" + bmLocation + "'");
      addLog("[BM] DBG rxFreq=" + String(bmRxFreq) + " txFreq=" + String(bmTxFreq));
    }

    bmSendRptl();
    bmState = BmState::WAITING_LOGIN;
    addLog("[BM] RPTL sent, waiting for login ACK...");

    unsigned long lastKeepalive = millis();
    unsigned long lastActivity = millis();
    bool running = true;

    while (running && bmEnabled) {
      int pktSize = dmrUdp.parsePacket();
      if (pktSize > 0) {
        uint8_t buf[512];
        int len = dmrUdp.read(buf, sizeof(buf));
        lastActivity = millis();

        // MSTNAK — BrandMeister rejected our packet
        if (len >= 6 && memcmp(buf, "MSTNAK", 6) == 0) {
          const char* stage = (bmState == BmState::WAITING_LOGIN) ? "LOGIN" : (bmState == BmState::WAITING_AUTH)   ? "AUTH"
                                                                            : (bmState == BmState::WAITING_CONFIG) ? "CONFIG"
                                                                                                                   : "UNKNOWN";
          addLog(String("[BM] NAK at stage: ") + stage + " — check callsign/ID");
          bmStatus = "NAK - check callsign";
          running = false;
        }
        // RPTACK — drives login state machine
        else if (len >= 10 && memcmp(buf, "RPTACK", 6) == 0) {
          switch (bmState) {
            case BmState::WAITING_LOGIN:
              memcpy(dmrSalt, buf + 6, 4);
              addLog("[BM] Login ACK received, sending auth...");
              bmState = BmState::WAITING_AUTH;
              bmSendRptk(dmrSalt);
              break;
            case BmState::WAITING_AUTH:
              addLog("[BM] Auth ACK, sending config...");
              bmState = BmState::WAITING_CONFIG;
              bmSendRptc();
              break;
            case BmState::WAITING_CONFIG:
              bmLoggedIn = true;
              bmStatus = "Connected";
              bmState = BmState::CONNECTED;
              lastKeepalive = millis();
              addLog("[BM] CONNECTED to BrandMeister!");
              break;
            default:
              break;
          }
        }
        // MSTPONG — keepalive ack, nothing to do
        else if (len >= 7 && memcmp(buf, "MSTPONG", 7) == 0) {
          // ok
        }
        // MSTCL — server closing connection
        else if (len >= 5 && memcmp(buf, "MSTCL", 5) == 0) {
          addLog("[BM] Server closed connection (MSTCL)");
          bmStatus = "Closed by server";
          bmLoggedIn = false;
          running = false;
        }
        // DMRD — incoming DMR frame, forward over ESP-NOW
        else if (len >= 55 && memcmp(buf, "DMRD", 4) == 0 && bmState == BmState::CONNECTED) {
          uint32_t srcId = ((uint32_t)buf[5] << 16) | ((uint32_t)buf[6] << 8) | buf[7];
          uint32_t dstId = ((uint32_t)buf[8] << 16) | ((uint32_t)buf[9] << 8) | buf[10];
          bool isGroup = (buf[15] & 0x40) == 0;
          uint8_t slot = (buf[15] & 0x80) ? 2 : 1;
          // Update DMRD TX info for UI card
          dmrdTxInfo.srcId = srcId;
          dmrdTxInfo.dstId = dstId;
          dmrdTxInfo.slot = slot;
          dmrdTxInfo.isGroup = isGroup;
          dmrdTxInfo.active = true;
          dmrdTxInfo.lastUpdate = millis();
          if (bmDebug) {
            addLog("[BM] DMRD src=" + String(srcId) + " dst=" + (isGroup ? "TG" : "") + String(dstId) + " slot=" + String(slot) + " → ESP-NOW");
          }
          espnowSendDmr(buf, (uint8_t)len);
        }
      }

      // Send keepalive
      if (bmState == BmState::CONNECTED && millis() - lastKeepalive > KEEPALIVE_MS) {
        bmSendKeepalive();
        lastKeepalive = millis();
      }

      // Login timeout
      if (bmState != BmState::CONNECTED && millis() - lastActivity > LOGIN_TO_MS) {
        addLog("[BM] Login timeout, retry in " + String(RECONNECT_MS / 1000) + "s");
        bmStatus = "Login timeout";
        running = false;
      }

      vTaskDelay(pdMS_TO_TICKS(10));
    }

    dmrUdp.stop();
    bmLoggedIn = false;
    if (bmStatus != "Closed by server") bmStatus = "Reconnecting...";
    vTaskDelay(pdMS_TO_TICKS(RECONNECT_MS));
  }
}


// ══════════════════════════════════════════════════════════════════════════
// DAPNET TASK
//
// Protocol (TCP, default port 43434):
//   Client → "[ESP-NOW-TX v<ver> <callsign> <authkey>]\r\n"
//   Server → "2:<hex>" (×5)   Client → "<line>:0000\r\n+\r\n"
//   Server → "3:<hex>"         Client → "+\r\n"
//   Server → "4:<hex>"         Client → "+\r\n"  (handshake done)
//   Server → "#<ctr_hex> 6:1:<RIC_hex>:<func>:<message>"
//   Client → "#<nextctr_hex> +\r\n"
// ══════════════════════════════════════════════════════════════════════════

static String dapnetReadLine(WiFiClient& tcp, unsigned long timeoutMs) {
  unsigned long t = millis();
  while (millis() - t < timeoutMs) {
    if (tcp.available()) {
      String line = tcp.readStringUntil('\n');
      line.trim();
      return line;
    }
    // Stop waiting only when disconnected AND no data left in buffer
    if (!tcp.connected() && !tcp.available()) break;
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  return "";
}

void dapnetTask(void* param) {
  addLog("[DAPNET] Task started");
  for (;;) {
    if (!dapnetEnabled) {
      dapnetStatus = "Disabled";
      vTaskDelay(pdMS_TO_TICKS(5000));
      continue;
    }

    while (WiFi.status() != WL_CONNECTED) {
      dapnetStatus = "Waiting for WiFi";
      vTaskDelay(pdMS_TO_TICKS(2000));
    }

    String cs = (dapnetCallsign.length() > 0) ? dapnetCallsign : bmCallsign;
    cs.toUpperCase();

    dapnetStatus = "Resolving...";
    addLog("[DAPNET] Resolving " + dapnetServer);

    IPAddress serverIP;
    if (!WiFi.hostByName(dapnetServer.c_str(), serverIP)) {
      addLog("[DAPNET] DNS resolve failed for " + dapnetServer + ", retry in 30s");
      dapnetStatus = "DNS failed";
      vTaskDelay(pdMS_TO_TICKS(30000));
      continue;
    }
    addLog("[DAPNET] Resolved " + dapnetServer + " → " + serverIP.toString());

    dapnetStatus = "Connecting...";
    addLog("[DAPNET] TCP connect to " + serverIP.toString() + ":" + String(dapnetPort) + " as " + cs);

    WiFiClient tcp;
    if (!tcp.connect(serverIP, dapnetPort, 15000)) {
      addLog("[DAPNET] TCP connect failed, retry in 30s");
      dapnetStatus = "Connect failed";
      vTaskDelay(pdMS_TO_TICKS(30000));
      continue;
    }
    addLog("[DAPNET] TCP connected");

    // Send login line
    String login = "[ESP-NOW-TX v" FW_VERSION " " + cs + " " + dapnetAuthKey + "]\r\n";
    tcp.print(login);
    addLog("[DAPNET] Login sent for " + cs);

    // Handshake
    bool loginOk = false;
    for (int step = 0; step < 20 && tcp.connected(); step++) {
      String line = dapnetReadLine(tcp, 5000);
      if (line.length() == 0) {
        addLog("[DAPNET] Handshake timeout at step " + String(step));
        break;
      }
      if (line.startsWith("7")) {
        addLog("[DAPNET] Login rejected: " + line);
        break;
      }
      if (line.startsWith("2:")) {
        tcp.print(line + ":0000\r\n");
        tcp.print("+\r\n");
        continue;
      }
      if (line.startsWith("3:")) {
        tcp.print("+\r\n");
        continue;
      }
      if (line.startsWith("4:")) {
        tcp.print("+\r\n");
        addLog("[DAPNET] Handshake OK — listening for pages");
        loginOk = true;
        break;
      }
    }

    if (!loginOk) {
      addLog("[DAPNET] Login failed, retry in 60s");
      dapnetStatus = "Login failed";
      dapnetLoggedIn = false;
      tcp.stop();
      vTaskDelay(pdMS_TO_TICKS(60000));
      continue;
    }

    dapnetLoggedIn = true;
    dapnetStatus = "Connected";

    // Small yield so lwIP can update the socket state after handshake
    vTaskDelay(pdMS_TO_TICKS(100));
    addLog("[DAPNET] Page loop start — connected=" + String(tcp.connected()) + " avail=" + String(tcp.available()));

    // Page receive loop — use available() too so we drain buffer even after server sends FIN
    while ((tcp.connected() || tcp.available()) && dapnetEnabled) {
      String line = dapnetReadLine(tcp, 30000);
      if (line.length() == 0) {
        if (!tcp.connected() && !tcp.available())
          addLog("[DAPNET] Connection closed by server");
        continue;
      }

      // Time-sync keepalives from server — must ACK or server drops us after ~60 min
      if (line.startsWith("2:")) {
        tcp.print(line + ":0000\r\n");
        tcp.print("+\r\n");
        continue;
      }
      if (line.startsWith("3:") || line.startsWith("4:") || line.startsWith("5:")) {
        tcp.print("+\r\n");
        continue;
      }

      if (!line.startsWith("#")) continue;

      // Format: "#<ctr_hex> 6:1:<RIC_hex>:<func>:<message>"
      int spacePos = line.indexOf(' ');
      if (spacePos < 2) continue;
      String ctrStr = line.substring(1, spacePos);
      int counter = (int)strtol(ctrStr.c_str(), nullptr, 16);
      String rest = line.substring(spacePos + 1);

      // Parse and forward page if type-6; always ACK
      // rest = "6:1:<RIC_hex>:<func>:<message>"
      //   c1 = after "6", c2 = after "1", c3 = after RIC, c4 = after func
      {
        int c1 = rest.indexOf(':');
        if (c1 >= 0 && rest.substring(0, c1).toInt() == 6) {
          int c2 = rest.indexOf(':', c1 + 1);
          int c3 = (c2 >= 0) ? rest.indexOf(':', c2 + 1) : -1;
          int c4 = (c3 >= 0) ? rest.indexOf(':', c3 + 1) : -1;
          if (c2 >= 0 && c3 >= 0 && c4 >= 0) {
            uint32_t ric = (uint32_t)strtoul(rest.substring(c2 + 1, c3).c_str(), nullptr, 16);
            uint8_t func = (uint8_t)rest.substring(c3 + 1, c4).toInt();
            String msg = rest.substring(c4 + 1);
            msg.trim();
            addLog("[DAPNET] Page RIC=" + String(ric) + " func=" + String(func) + " → ESP-NOW | " + msg);
            addDapnetPage(ric, func, msg);
            espnowSendPocsag(ric, func, msg.c_str());
          }
        } else if (c1 >= 0) {
          if (dapnetDebug)
            addLog("[DAPNET] Non-page type " + rest.substring(0, c1) + ", ctr=" + ctrStr);
        }
        // ACK every numbered frame
        uint8_t nextCtr = (uint8_t)(counter + 1);
        char ack[16];
        snprintf(ack, sizeof(ack), "#%02X +\r\n", nextCtr);
        tcp.print(ack);
      }
    }

    dapnetLoggedIn = false;
    dapnetStatus = "Disconnected";
    tcp.stop();
    addLog("[DAPNET] Disconnected, retry in 30s");
    vTaskDelay(pdMS_TO_TICKS(30000));
  }
}


// ══════════════════════════════════════════════════════════════════════════
// WEB INTERFACE
// ══════════════════════════════════════════════════════════════════════════

static String badge(const String& status) {
  String cls;
  if (status == "Connected" || status == "Ready") cls = "badge-success";
  else if (status == "Disabled" || status == "Not configured") cls = "badge-secondary";
  else if (status.indexOf("fail") >= 0 || status.indexOf("reject") >= 0 || status.indexOf("timeout") >= 0 || status.indexOf("DNS") >= 0) cls = "badge-danger";
  else cls = "badge-warning";
  return "<span class='status-badge " + cls + "'>" + status + "</span>";
}

String getPageHTML() {
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  String ip = wifiOk ? WiFi.localIP().toString() : "—";
  String mac = WiFi.macAddress();
  String wifiStatus = wifiOk ? "Connected" : "Connecting...";

  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>ESP-NOW Transmitter</title>";

  // Theme init script (before styles so no flash)
  html += "<script>var t=localStorage.getItem('theme')||'light';document.documentElement.setAttribute('data-theme',t);</script>";
  html += "<style>" + getStyles() + "</style>";
  html += "</head><body>";

  // ── Navbar ────────────────────────────────────────────────────────────
  html += "<nav class='navbar'>";
  html += "<div><div class='nav-brand'>ESP-NOW Transmitter</div>"
    "<div class='nav-sub'>BrandMeister + DAPNET → ESP-NOW bridge &nbsp;·&nbsp; v" FW_VERSION "</div></div>";
  html += "<button class='theme-toggle' onclick='toggleTheme()' title='Toggle dark/light'>&#127769;</button>";
  html += "</nav>";

  // ── Page ──────────────────────────────────────────────────────────────
  html += "<div class='container'>";
  html += "<h1>Configuration</h1>";

  // ── Status overview ───────────────────────────────────────────────────
  html += "<div class='admin-grid'>";

  // WiFi — always
  html += "<div class='card'><h3>WiFi</h3>";
  html += "<div class='metric'><span class='metric-label'>Status:</span>" + badge(wifiStatus) + "</div>";
  html += "<div class='metric'><span class='metric-label'>IP address:</span><span style='font-family:monospace'>" + ip + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>MAC address:</span><span style='font-family:monospace;font-size:.9em'>" + mac + "</span></div>";
  html += "</div>";

  // ESP-NOW — always, next to WiFi
  {
    String enStatus = espnowReady ? "Ready" : (espnowMacs.length() > 0 ? "Pending reboot" : "Not configured");
    html += "<div class='card'><h3>ESP-NOW</h3>";
    html += "<div class='metric'><span class='metric-label'>Peers:</span><span>" + String(espnowPeerCount) + " configured</span></div>";
    html += "<div class='metric'><span class='metric-label'>Status:</span>" + badge(enStatus) + "</div>";
    html += "</div>";
  }

  // BrandMeister — only when enabled
  if (bmEnabled) {
    html += "<div class='card'><h3>BrandMeister</h3>";
    html += "<div class='metric'><span class='metric-label'>Status:</span>" + badge(bmStatus) + "</div>";
    html += "<div class='metric'><span class='metric-label'>DMR ID:</span><span>" + String(bmDmrId) + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Callsign:</span><span>" + bmCallsign + (bmSsid > 0 ? "-" + String(bmSsid) : "") + "</span></div>";
    html += "</div>";

    // DMR Activity — live TX info
    html += "<div class='card' id='dmrd-tx-card'><h3>DMR Activity</h3>";
    html += "<div id='dmrd-tx-info'><span style='color:#888'>No transmission yet.</span></div>";
    html += "</div>";
  }

  // DAPNET + Recent Pages — only when enabled
  if (dapnetEnabled) {
    String dpCs = dapnetCallsign.length() > 0 ? dapnetCallsign : bmCallsign;
    html += "<div class='card'><h3>DAPNET</h3>";
    html += "<div class='metric'><span class='metric-label'>Status:</span>" + badge(dapnetStatus) + "</div>";
    html += "<div class='metric'><span class='metric-label'>Callsign:</span><span>" + dpCs + "</span></div>";
    html += "</div>";

    html += "<div class='card' id='dapnet-pages-card'><h3>DAPNET Recent Pages</h3>";
    html += "<div id='dp-pages-list' style='font-size:.88em'>"
      "<span style='color:#888'>No pages received yet.</span></div>";
    html += "</div>";
  }

  html += "</div>";  // status admin-grid

  // ── BrandMeister settings ─────────────────────────────────────────────
  html += "<div class='admin-grid'>";
  // BrandMeister card with gray-out logic (C++ only)
  html += "<div class='card' id='bm-card'><h3>BrandMeister (DMR)</h3>";
  html += "<div class='metric'><span class='metric-label'>Enable:</span>"
    "<label class='switch'><input type='checkbox' id='bm-en'"
    + String(bmEnabled ? " checked" : "") + "><span class='slider'></span></label></div>";
  String bmFieldsStyle = dapnetEnabled ? " style='opacity:0.5;'" : "";
  html += "<div class='bm-fields'" + bmFieldsStyle + ">";
  html += "<div class='metric'><span class='metric-label'>DMR ID:</span>"
    "<input type='number' id='bm-id' value='"
    + String(bmDmrId) + "' min='1' max='16776415'></div>";
  html += "<div class='metric'><span class='metric-label'>Callsign:</span>"
    "<input type='text' id='bm-cs' value='"
    + bmCallsign + "' maxlength='8'></div>";
  html += "<div class='metric'><span class='metric-label'>SSID (0-99):</span>"
    "<input type='number' id='bm-ssid' value='"
    + String(bmSsid) + "' min='0' max='99' style='max-width:80px'></div>";
  html += "<div style='margin:6px 0 2px'>"
    "<button id='bm-adv-btn' onclick='toggleBmAdvanced()'"
    " style='width:100%;font-size:0.82em;padding:4px 0;cursor:pointer;background:none;"
    "border:1px solid var(--border,#ccc);border-radius:4px;color:inherit'>"
    "&#9660; Advanced settings</button></div>";
  html += "<div id='bm-advanced' style='display:none'>";
  html += "<div class='metric'><span class='metric-label'>Password:</span>"
    "<input type='password' id='bm-pass' value='"
    + bmPassword + "'></div>";
  {
    bool isKnown = false;
    for (int i = 0; i < bmServerCount; i++) {
      if (bmServer == bmServers[i].address) {
  isKnown = true;
  break;
      }
    }
    html += "<div class='metric'><span class='metric-label'>Server:</span>"
      "<select id='bm-server-sel' onchange='bmServerSel()' style='width:100%;margin-bottom:4px'>";
    html += String("<option value='custom'") + (isKnown ? ">" : " selected>") + "Custom (enter below)</option>";
    for (int i = 0; i < bmServerCount; i++) {
      html += "<option value='" + String(bmServers[i].address) + "'" + (bmServer == bmServers[i].address ? " selected" : "") + ">" + String(bmServers[i].name) + "</option>";
    }
    html += "</select>"
      "<input type='text' id='bm-server' value='"
      + bmServer + "' class='full' placeholder='hostname or IP'></div>";
  }
  html += "<div class='metric'><span class='metric-label'>Port:</span>"
    "<input type='number' id='bm-port' value='"
    + String(bmPort) + "' style='max-width:100px'></div>";
  html += "<div class='metric'><span class='metric-label'>Location:</span>"
    "<input type='text' id='bm-loc' value='"
    + bmLocation + "' class='full'></div>";
  html += "<div class='metric'><span class='metric-label'>Latitude:</span>"
    "<input type='text' id='bm-lat' value='"
    + bmLatitude + "' style='max-width:140px'></div>";
  html += "<div class='metric'><span class='metric-label'>Longitude:</span>"
    "<input type='text' id='bm-lon' value='"
    + bmLongitude + "' style='max-width:140px'></div>";
  html += "<div class='metric'><span class='metric-label'>Height (m):</span>"
    "<input type='number' id='bm-h' value='"
    + String(bmHeight) + "' min='0' max='999' style='max-width:100px'></div>";
  html += "<div class='metric'><span class='metric-label'>RX Freq (Hz):</span>"
    "<input type='number' id='bm-rxf' value='"
    + String(bmRxFreq) + "' min='100000000' max='500000000'></div>";
  html += "<div class='metric'><span class='metric-label'>TX Freq (Hz):</span>"
    "<input type='number' id='bm-txf' value='"
    + String(bmTxFreq) + "' min='100000000' max='500000000'></div>";
  html += "<p style='font-size:0.82em;color:#888;margin-top:2px;margin-bottom:4px'>Must match your registered hotspot frequency. BrandMeister rejects freq=0.</p>";
  html += "<div class='metric'><span class='metric-label'>Debug Logging:</span>"
    "<label class='switch'><input type='checkbox' id='bm-debug'"
    + String(bmDebug ? " checked" : "") + "><span class='slider'></span></label></div>";
  html += "<p style='font-size:0.82em;color:#888;margin-top:4px;'>Log callsign, password hint, and config values on each connect attempt.</p>";
  html += "</div>";  // end bm-advanced
  html += "<div class='action-buttons-vertical'>";
  html += "<button class='btn btn-success' onclick='saveBoth()'>Save BrandMeister & DAPNET settings</button>";
  html += "</div>";
  html += "</div></div>";

  // ── DAPNET settings ───────────────────────────────────────────────────
  // DAPNET card with gray-out logic (C++ only)
  html += "<div class='card' id='dp-card'><h3>DAPNET (POCSAG)</h3>";
  html += "<div class='metric'><span class='metric-label'>Enable:</span>"
    "<label class='switch'><input type='checkbox' id='dp-en'"
    + String(dapnetEnabled ? " checked" : "") + "><span class='slider'></span></label></div>";
  String dpFieldsStyle = bmEnabled ? " style='opacity:0.5;'" : "";
  html += "<div class='dp-fields'" + dpFieldsStyle + ">";
  html += "<div class='metric'><span class='metric-label'>Callsign:</span>"
    "<input type='text' id='dp-cs' placeholder='(same as BM)' value='"
    + dapnetCallsign + "' maxlength='10'></div>";
  html += "<div class='metric'><span class='metric-label'>Auth Key:</span>"
    "<input type='password' id='dp-key' value='"
    + dapnetAuthKey + "' class='full'></div>";
  html += "<div class='metric'><span class='metric-label'>Server:</span>"
    "<input type='text' id='dp-server' value='"
    + dapnetServer + "' class='full'></div>";
  html += "<div class='metric'><span class='metric-label'>Port:</span>"
    "<input type='number' id='dp-port' value='"
    + String(dapnetPort) + "' style='max-width:100px'></div>";
  html += "<div class='metric'><span class='metric-label'>Debug Logging:</span>"
    "<label class='switch'><input type='checkbox' id='dp-debug'"
    + String(dapnetDebug ? " checked" : "") + "><span class='slider'></span></label></div>";
  html += "<p style='font-size:0.82em;color:#888;margin-top:4px;'>Log rubric/non-page frames (type 5, etc.). Disable unless debugging.</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button class='btn btn-success' onclick='saveBoth()'>Save BrandMeister & DAPNET settings</button>";
  html += "</div>";
  html += "</div></div>";
  // Unified save button for both BrandMeister and DAPNET
  // html +="<div class='action-buttons-vertical'>"
  //      "<button class='btn btn-success' onclick='saveBoth()'>Save BrandMeister & DAPNET settings</button>"
  //      "</div>";


  // ── ESP-NOW receivers ─────────────────────────────────────────────────
  html += "<div class='card'><h3>ESP-NOW Receivers</h3>";
  html += "<div class='metric'><span class='metric-label'>Receiver MAC(s):</span></div>";
  String macs[6] = { "", "", "", "", "", "" };
  int macIdx = 0, start = 0;
  for (int i = 0; i < espnowMacs.length() && macIdx < 6; ++i) {
    if (espnowMacs[i] == ',' || i == espnowMacs.length() - 1) {
      int end = (espnowMacs[i] == ',') ? i : i + 1;
      String mac = espnowMacs.substring(start, end);
      mac.trim();
      macs[macIdx++] = mac;
      start = i + 1;
    }
  }
  for (int i = 0; i < 6; ++i) {
    html += "<div class='metric'><input type='text' id='en-mac" + String(i + 1) + "' value='" + macs[i] + "' placeholder='AA:BB:CC:DD:EE:FF' style='font-family:monospace;max-width:220px;margin-bottom:2px'></div>";
  }
  html += "<div class='info' style='margin-top:10px'>Up to 6 MAC addresses. "
    "Use <b>Scan</b> to auto-discover receivers on the network, or enter MACs manually. "
    "MAC changes require a reboot to apply.</div>";
  html += "<div style='margin:10px 0'>"
    "<button id='en-scan-btn' class='btn btn-primary' style='width:100%' onclick='scanEspNow()'>"
    "&#128268; Scan for receivers</button>"
    "<div id='en-scan-results' style='margin-top:8px;font-size:.88em'></div></div>";
  html += "<div class='action-buttons-vertical'>"
    "<button class='btn btn-success' onclick='saveEn()'>Save ESP-NOW receivers</button>"
    "<button class='btn btn-danger' onclick='reboot()'>Reboot device</button>"
    "</div></div>";


  html += "</div>";  // config admin-grid

  // ── Log ───────────────────────────────────────────────────────────────
  html += "<div class='card' style='margin-top:20px'>";
  html += "<h3>Log <button class='btn btn-primary' style='float:right;padding:4px 12px;font-size:13px' onclick='fetchLog()'>Refresh</button></h3>";
  html += "<pre id='log' style='max-height:260px'></pre>";
  html += "</div>";
  // Footer
  html += "<br>";
  html += "<div class='footer-links'>" FOOTER_LINKS "</div>";
  html += "<div class='copyright'>" FOOTER_COPYRIGHT "</div>";

  // ── Scripts ───────────────────────────────────────────────────────────
  html += "<script>";
  // Mutual exclusivity logic with live card enable/disable
  html += "document.addEventListener('DOMContentLoaded',function(){\n"
    "var bm=document.getElementById('bm-en');\n"
    "var dp=document.getElementById('dp-en');\n"
    "var bmFields=document.querySelector('.bm-fields');\n"
    "var dpFields=document.querySelector('.dp-fields');\n"
    "function setCardState(fieldsDiv, enable){\n"
    "  fieldsDiv.style.opacity=enable?1:0.5;\n"
    "  var fields=fieldsDiv.querySelectorAll('input,button');\n"
    "  for(var i=0;i<fields.length;i++){\n"
    "    if(fields[i].id==='bm-en'||fields[i].id==='dp-en') continue;\n"
    "    fields[i].disabled=!enable;\n"
    "    if(fields[i].tagName==='BUTTON'){fields[i].style.opacity=enable?1:0.5;}\n"
    "  }\n"
    "}\n"
    "function syncCards(){\n"
    "  bm.disabled=false;\n"
    "  dp.disabled=false;\n"
    "  if(dp.checked){\n"
    "    setCardState(bmFields,false);\n"
    "    setCardState(dpFields,true);\n"
    "  }else if(bm.checked){\n"
    "    setCardState(bmFields,true);\n"
    "    setCardState(dpFields,false);\n"
    "  }else{\n"
    "    setCardState(bmFields,true);\n"
    "    setCardState(dpFields,true);\n"
    "  }\n"
    "}\n"
    "bm.addEventListener('change',syncCards);\n"
    "dp.addEventListener('change',syncCards);\n"
    "syncCards();\n"
    "});\n";

  // Theme toggle
  html += "function bmServerSel(){"
    "var s=document.getElementById('bm-server-sel').value;"
    "var i=document.getElementById('bm-server');"
    "if(s!=='custom'){i.value=s;}else{i.value='';i.focus();}}";
  html += "function toggleBmAdvanced(){"
    "var d=document.getElementById('bm-advanced');"
    "var b=document.getElementById('bm-adv-btn');"
    "var open=d.style.display==='none';"
    "d.style.display=open?'block':'none';"
    "b.innerHTML=open?'&#9650; Advanced':'&#9660; Advanced';}";
  html += "function toggleTheme(){"
    "var html=document.documentElement;"
    "var t=html.getAttribute('data-theme')==='dark'?'light':'dark';"
    "html.setAttribute('data-theme',t);localStorage.setItem('theme',t);}";

  // Modal helpers — identical pattern to main project
  html += "window.showModal=function(fn){"
    "var o=document.createElement('div');o.className='modal-overlay';"
    "var b=document.createElement('div');b.className='modal-box';"
    "fn(b,function(){document.body.removeChild(o);});"
    "o.appendChild(b);"
    "o.addEventListener('click',function(e){if(e.target===o)document.body.removeChild(o);});"
    "document.body.appendChild(o);};"
    "window.showAlert=function(msg){"
    "showModal(function(b,close){"
    "b.innerHTML='<h4>'+msg+'</h4>';"
    "var d=document.createElement('div');d.className='modal-buttons';"
    "var ok=document.createElement('button');ok.textContent='OK';ok.className='btn btn-primary';ok.onclick=close;"
    "d.appendChild(ok);b.appendChild(d);});};"
    "window.showConfirm=function(msg,onYes){"
    "showModal(function(b,close){"
    "b.innerHTML='<h4>'+msg+'</h4>';"
    "var d=document.createElement('div');d.className='modal-buttons';"
    "var y=document.createElement('button');y.textContent='Yes';y.className='btn btn-success';"
    "y.onclick=function(){close();onYes();};"
    "var n=document.createElement('button');n.textContent='Cancel';n.className='btn btn-danger';n.onclick=close;"
    "d.appendChild(y);d.appendChild(n);b.appendChild(d);});};";

  // Unified save for both BrandMeister and DAPNET
  html += "function saveBoth(){"
    "var b={\n"
    "'bm_en':document.getElementById('bm-en').checked?'1':'0',\n"
    "'bm_id':document.getElementById('bm-id').value,\n"
    "'bm_cs':document.getElementById('bm-cs').value.trim().toUpperCase(),\n"
    "'bm_ssid':document.getElementById('bm-ssid').value,\n"
    "'bm_pass':document.getElementById('bm-pass').value,\n"
    "'bm_server':document.getElementById('bm-server').value.trim(),\n"
    "'bm_port':document.getElementById('bm-port').value,\n"
    "'bm_loc':document.getElementById('bm-loc').value.trim(),\n"
    "'bm_lat':document.getElementById('bm-lat').value.trim(),\n"
    "'bm_lon':document.getElementById('bm-lon').value.trim(),\n"
    "'bm_h':document.getElementById('bm-h').value,\n"
    "'bm_rxf':document.getElementById('bm-rxf').value,\n"
    "'bm_txf':document.getElementById('bm-txf').value,\n"
    "'dp_en':document.getElementById('dp-en').checked?'1':'0',\n"
    "'dp_cs':document.getElementById('dp-cs').value.trim().toUpperCase(),\n"
    "'dp_key':document.getElementById('dp-key').value.trim(),\n"
    "'dp_server':document.getElementById('dp-server').value.trim(),\n"
    "'dp_port':document.getElementById('dp-port').value,\n"
    "'dp_debug':document.getElementById('dp-debug').checked?'1':'0',\n"
    "'bm_debug':document.getElementById('bm-debug').checked?'1':'0'\n"
    "};\n"
    "var body=Object.keys(b).map(k=>encodeURIComponent(k)+'='+encodeURIComponent(b[k])).join('&');\n"
    "showConfirm('Save BrandMeister & DAPNET settings?',function(){\n"
    "fetch('/api/save-both',{method:'POST',\n"
    "headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})\n"
    ".then(r=>r.text()).then(function(msg){\n"
    "  showAlert(msg);\n"
    "  var okBtn=document.querySelector('.modal-box .btn-primary');\n"
    "  if(okBtn){okBtn.addEventListener('click',function(){\n"
    "    if(msg.indexOf('saved. Reboot to re-connect')!==-1){\n"
    "      // Show reboot overlay\n"
    "      var overlay=document.createElement('div');\n"
    "      overlay.style.position='fixed';\n"
    "      overlay.style.top=0;overlay.style.left=0;overlay.style.width='100vw';overlay.style.height='100vh';\n"
    "      overlay.style.background='rgba(255,255,255,0.96)';\n"
    "      overlay.style.zIndex=9999;overlay.style.display='flex';overlay.style.flexDirection='column';overlay.style.justifyContent='center';overlay.style.alignItems='center';\n"
    "      overlay.innerHTML='<div style=\\'font-size:2em;margin-bottom:16px;color:#333\\'>Rebooting...<br><span style=\\'font-size:0.7em;color:#888\\'>(please wait 10 seconds)</span></div><div class=\\'loader\\'></div>';\n"
    "      document.body.appendChild(overlay);\n"
    "      fetch('/api/reboot',{method:'POST'});\n"
    "      setTimeout(function(){location.reload();},10000);\n"
    "    }\n"
    "  });}\n"
    "});\n"
    "});}";

  // ESP-NOW save (combine 6 fields into comma-delimited string)
  html += "function saveEn(){"
    "var macs=[];"
    "for(var i=1;i<=6;i++){"
    "  var v=document.getElementById('en-mac'+i).value.trim().toUpperCase();"
    "  if(v) macs.push(v);"
    "}"
    "var re=/^([0-9A-F]{2}:){5}[0-9A-F]{2}$/;"
    "for(var i=0;i<macs.length;i++){"
    "  if(!re.test(macs[i])){showAlert('Invalid MAC #'+(i+1)+': use AA:BB:CC:DD:EE:FF');return;}"
    "}"
    "if(macs.length>6){showAlert('Maximum 6 MACs allowed');return;}"
    "showConfirm('Save ESP-NOW receiver MACs?<br>Reboot required to apply.',function(){"
    "fetch('/api/save-en',{method:'POST',"
    "headers:{'Content-Type':'application/x-www-form-urlencoded'},"
    "body:'en_macs='+encodeURIComponent(macs.join(','))})"
    ".then(r=>r.text()).then(showAlert);});}";

  // Reboot
  html += "function reboot(){"
    "showConfirm('Reboot device now?',function(){"
    "fetch('/api/reboot',{method:'POST'}).then(function(){"
    "showAlert('Rebooting... page will reload in 12s.');"
    "setTimeout(function(){location.reload();},12000);});});}";

  // DAPNET pages fetch — auto-refresh every 5 s
  html += "function fmtAge(s){"
    "if(s<60)return s+'s ago';"
    "var m=Math.floor(s/60),r=s%60;"
    "if(m<60)return m+'m '+r+'s ago';"
    "return Math.floor(m/60)+'h '+m%60+'m ago';}"
    "function fetchDapnetPages(){"
    "fetch('/api/dapnet-pages').then(r=>r.json()).then(function(d){"
    "var el=document.getElementById('dp-pages-list');"
    "if(!d.pages||d.pages.length===0){"
    "el.innerHTML='<span style=\"color:#888\">No pages received yet.</span>';return;}"
    "var rows=d.pages.map(function(p){"
    "return '<div style=\"border-bottom:1px solid var(--border,#ddd);padding:4px 0\">'"
    "+'<span style=\"font-family:monospace;font-weight:600\">RIC '+p.ric+'</span>'"
    "+'<span style=\"color:#888;font-size:.85em;margin-left:8px\">func '+p.func+'</span>'"
    "+'<span style=\"color:#888;font-size:.82em;float:right\">'+fmtAge(p.age_s)+'</span>'"
    "+'<div style=\"word-break:break-all;margin-top:2px\">'+p.msg+'</div></div>';});"
    "el.innerHTML=rows.join('');}).catch(function(){});}";
  html += "fetchDapnetPages();setInterval(fetchDapnetPages,5000);";

  // DMR Activity polling — 1 s, only when BM enabled
  if (bmEnabled) {
    html += "function fetchDmrdTx(){"
      "fetch('/api/dmrd-tx').then(r=>r.json()).then(function(d){"
      "var el=document.getElementById('dmrd-tx-info');if(!el)return;"
      "var dst=d.isGroup?'TG '+d.dst:'PC '+d.dst;"
      "if(d.active){"
      "el.innerHTML="
        "'<div class=\"metric\"><span class=\"metric-label\">Status:</span>"
        "<span class=\"status-badge badge-success\">Transmitting</span></div>'"
        "+'<div class=\"metric\"><span class=\"metric-label\">From:</span>"
        "<span style=\"font-family:monospace;font-weight:600\">'+d.src+'</span></div>'"
        "+'<div class=\"metric\"><span class=\"metric-label\">To:</span>"
        "<span style=\"font-family:monospace;font-weight:600\">'+dst+'</span></div>'"
        "+'<div class=\"metric\"><span class=\"metric-label\">Slot:</span><span>'+d.slot+'</span></div>';"
      "}else if(d.src>0){"
      "el.innerHTML="
        "'<div class=\"metric\"><span class=\"metric-label\">Status:</span>"
        "<span class=\"status-badge badge-secondary\">Idle</span></div>'"
        "+'<div style=\"opacity:.5;font-size:.9em\">'"
        "+'<div class=\"metric\"><span class=\"metric-label\">Last from:</span>"
        "<span style=\"font-family:monospace\">'+d.src+'</span></div>'"
        "+'<div class=\"metric\"><span class=\"metric-label\">Last to:</span>"
        "<span style=\"font-family:monospace\">'+dst+'</span></div>'"
        "+'<div class=\"metric\"><span class=\"metric-label\">Slot:</span><span>'+d.slot+'</span></div></div>';"
      "}else{"
      "el.innerHTML='<span style=\"color:#888\">No transmission yet.</span>';"
      "}}).catch(function(){});}";
    html += "fetchDmrdTx();setInterval(fetchDmrdTx,1000);";
  }

  // ESP-NOW scan
  html += "function scanEspNow(){"
    "var btn=document.getElementById('en-scan-btn');"
    "var res=document.getElementById('en-scan-results');"
    "btn.disabled=true;btn.textContent='Scanning...';"
    "res.innerHTML='<span style=\"color:#888\">Scanning for 2 seconds\u2026</span>';"
    "fetch('/api/espnow-scan').then(r=>r.json()).then(function(d){"
    "btn.disabled=false;btn.innerHTML='&#128268; Scan for receivers';"
    "if(!d.found||d.found.length===0){"
    "res.innerHTML='<span style=\"color:#888\">No receivers found.</span>';return;}"
    "var rows=d.found.map(function(f){"
    "return '<div style=\"display:flex;align-items:center;gap:8px;margin:4px 0;flex-wrap:wrap\">'"
    "+'<span style=\"font-family:monospace\">'+f.mac+'</span>'"
    "+'<span style=\"color:#888;font-size:.85em\">'+f.name+'</span>'"
    "+'<button class=\"btn btn-success\" style=\"padding:2px 10px;font-size:.82em\" '"
    "+'onclick=\"addEspNowMac(\\'' + f.mac + '\\')\">Add</button></div>';});"
    "res.innerHTML=rows.join('');}).catch(function(){"
    "btn.disabled=false;btn.innerHTML='&#128268; Scan for receivers';"
    "res.innerHTML='<span style=\"color:#f44\">Scan failed.</span>';});}";
  html += "function addEspNowMac(mac){"
    "for(var i=1;i<=6;i++){"
    "var f=document.getElementById('en-mac'+i);"
    "if(f&&f.value.trim()===''&&f.value!==mac){f.value=mac;return;}}"
    "showAlert('All 6 MAC fields are already filled. Remove one first.');}";

  // Log fetch — auto-refresh every 5 s
  html += "function fetchLog(){"
    "fetch('/api/log').then(r=>r.json()).then(function(lines){"
    "var box=document.getElementById('log');"
    "box.textContent=lines.join('\\n');"
    "box.scrollTop=box.scrollHeight;}).catch(function(){});}";
  html += "fetchLog();setInterval(fetchLog,5000);";

  html += "</script>";
  html += "</div></body></html>";
  return html;
}


// ══════════════════════════════════════════════════════════════════════════
// WEB SERVER ROUTES
// ══════════════════════════════════════════════════════════════════════════

void registerRoutes() {
    // DMRD TX Info API
    server.on("/api/dmrd-tx", HTTP_GET, []() {
      String j = "{";
      j += "\"active\":" + String(dmrdTxInfo.active ? "true" : "false");
      j += ",\"src\":" + String(dmrdTxInfo.srcId);
      j += ",\"dst\":" + String(dmrdTxInfo.dstId);
      j += ",\"slot\":" + String(dmrdTxInfo.slot);
      j += ",\"isGroup\":" + String(dmrdTxInfo.isGroup ? "true" : "false");
      j += "}";
      server.send(200, "application/json", j);
    });
  // Main page
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", getPageHTML());
  });

  // DAPNET recent pages API
  server.on("/api/dapnet-pages", HTTP_GET, []() {
    portENTER_CRITICAL(&dapnetPageMux);
    unsigned long now = millis();
    String j = "{\"pages\":[";
    for (int i = 0; i < dapnetPageCount; i++) {
      // newest first: index from (head-1) backwards
      int idx = ((dapnetPageHead - 1 - i) % DAPNET_PAGE_HISTORY + DAPNET_PAGE_HISTORY) % DAPNET_PAGE_HISTORY;
      const DapnetPageEntry& e = dapnetPageHistory[idx];
      if (i > 0) j += ",";
      unsigned long age = (now >= e.rxMs) ? (now - e.rxMs) / 1000 : 0;
      String msgEsc = String(e.msg);
      msgEsc.replace("\\", "\\\\");
      msgEsc.replace("\"", "\\\"");
      j += "{\"ric\":" + String(e.ric)
        + ",\"func\":" + String(e.func)
        + ",\"msg\":\"" + msgEsc + "\""
        + ",\"age_s\":" + String(age) + "}";
    }
    j += "]}";
    portEXIT_CRITICAL(&dapnetPageMux);
    server.send(200, "application/json", j);
  });

  // ESP-NOW receiver discovery scan
  server.on("/api/espnow-scan", HTTP_GET, []() {
    WiFiUDP udp;
    udp.begin(3490);  // ephemeral receive port

    // Send broadcast discovery ping
    const char* ping = "{\"type\":\"ESPNOW_DISCOVER\"}";
    udp.beginPacket(IPAddress(255, 255, 255, 255), 3491);
    udp.write((const uint8_t*)ping, strlen(ping));
    udp.endPacket();

    struct Found { String mac; String name; } found[6];
    int foundCount = 0;
    unsigned long start = millis();

    while (millis() - start < 2000 && foundCount < 6) {
      int len = udp.parsePacket();
      if (len > 0) {
        char buf[256] = {};
        int got = udp.read(buf, min(len, 255));
        String s = String(buf).substring(0, got);
        int mi = s.indexOf("\"mac\":\"");
        int ni = s.indexOf("\"name\":\"");
        if (mi >= 0) {
          mi += 7;
          String mac = s.substring(mi, s.indexOf("\"", mi));
          String name = "Unknown";
          if (ni >= 0) { ni += 8; name = s.substring(ni, s.indexOf("\"", ni)); }
          bool dup = false;
          for (int i = 0; i < foundCount; i++) if (found[i].mac == mac) { dup = true; break; }
          if (!dup) { found[foundCount].mac = mac; found[foundCount].name = name; foundCount++; }
        }
      }
      delay(10);
    }
    udp.stop();

    String j = "{\"found\":[";
    for (int i = 0; i < foundCount; i++) {
      if (i > 0) j += ",";
      j += "{\"mac\":\"" + found[i].mac + "\",\"name\":\"" + found[i].name + "\"}";
    }
    j += "]}";
    server.send(200, "application/json", j);
  });

  // Log API
  server.on("/api/log", HTTP_GET, []() {
    server.send(200, "application/json", getLogsJson());
  });

  // Status API
  server.on("/api/status", HTTP_GET, []() {
    String j = "{";
    j += "\"wifi\":\"" + (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "") + "\"";
    j += ",\"bm\":\"" + bmStatus + "\"";
    j += ",\"dapnet\":\"" + dapnetStatus + "\"";
    j += ",\"espnow_peers\":" + String(espnowPeerCount);
    j += "}";
    server.send(200, "application/json", j);
  });

  // Unified save for both BrandMeister and DAPNET
  server.on("/api/save-both", HTTP_POST, []() {
    if (server.hasArg("bm_id")) bmDmrId = server.arg("bm_id").toInt();
    if (server.hasArg("bm_cs")) bmCallsign = server.arg("bm_cs");
    if (server.hasArg("bm_ssid")) bmSsid = (uint8_t)constrain(server.arg("bm_ssid").toInt(), 0, 99);
    if (server.hasArg("bm_pass")) bmPassword = server.arg("bm_pass");
    if (server.hasArg("bm_server")) bmServer = server.arg("bm_server");
    if (server.hasArg("bm_port")) bmPort = (uint16_t)server.arg("bm_port").toInt();
    if (server.hasArg("bm_loc")) bmLocation = server.arg("bm_loc");
    if (server.hasArg("bm_lat")) bmLatitude = server.arg("bm_lat");
    if (server.hasArg("bm_lon")) bmLongitude = server.arg("bm_lon");
    if (server.hasArg("bm_h")) bmHeight = server.arg("bm_h").toInt();
    if (server.hasArg("bm_rxf")) bmRxFreq = (uint32_t)server.arg("bm_rxf").toInt();
    if (server.hasArg("bm_txf")) bmTxFreq = (uint32_t)server.arg("bm_txf").toInt();
    if (server.hasArg("bm_en")) bmEnabled = (server.arg("bm_en") == "1");
    if (server.hasArg("bm_debug")) bmDebug = (server.arg("bm_debug") == "1");
    if (server.hasArg("dp_en")) dapnetEnabled = (server.arg("dp_en") == "1");
    if (server.hasArg("dp_cs")) dapnetCallsign = server.arg("dp_cs");
    if (server.hasArg("dp_key")) dapnetAuthKey = server.arg("dp_key");
    if (server.hasArg("dp_server")) dapnetServer = server.arg("dp_server");
    if (server.hasArg("dp_port")) dapnetPort = (uint16_t)server.arg("dp_port").toInt();
    if (server.hasArg("dp_debug")) dapnetDebug = (server.arg("dp_debug") == "1");
    saveSettings();
    addLog("[Web] BrandMeister & DAPNET settings saved");
    server.send(200, "text/plain", "BrandMeister & DAPNET settings saved. Reboot to re-connect.");
  });

  // Save ESP-NOW receivers
  server.on("/api/save-en", HTTP_POST, []() {
    if (server.hasArg("en_macs")) espnowMacs = server.arg("en_macs");
    saveSettings();
    addLog("[Web] ESP-NOW MACs saved: " + espnowMacs);
    server.send(200, "text/plain", "ESP-NOW MACs saved. Reboot to apply.");
  });

  // Reboot
  server.on("/api/reboot", HTTP_POST, []() {
    server.send(200, "text/plain", "Rebooting...");
    delay(500);
    ESP.restart();
  });

  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });
}


// ══════════════════════════════════════════════════════════════════════════
// ARDUINO ENTRY POINTS
// ══════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\n=== ESP-NOW Transmitter v" FW_VERSION " ===");

  // Load settings from NVS (defaults on first boot)
  loadSettings();
  saveSettings();  // ensure all keys exist in NVS

  // WiFi — station mode required for both ESP-NOW and internet access
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[WiFi] Connecting to %s...\n", WIFI_SSID);
  unsigned long wt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wt < 15000) {
    delay(250);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Connected: %s  MAC: %s\n",
                  WiFi.localIP().toString().c_str(),
                  WiFi.macAddress().c_str());
    addLog("[WiFi] Connected: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n[WiFi] Not connected yet — will retry in background");
    addLog("[WiFi] Not connected (will retry)");
  }

  // ESP-NOW (needs WiFi driver up)
  initEspNow();

  // Web server
  registerRoutes();
  server.begin();
  addLog("[Web] Server started on port 80");

  // BrandMeister task — core 0
  xTaskCreatePinnedToCore(bmTask, "bm", 6144, nullptr, 1, &bmTaskHandle, 0);

  // DAPNET task — core 0
  xTaskCreatePinnedToCore(dapnetTask, "dapnet", 6144, nullptr, 1, &dapnetTaskHandle, 0);

  Serial.println("[Setup] Done");
}

void loop() {
  server.handleClient();
  // Clear DMRD TX info if idle for >600ms
  if (dmrdTxInfo.active && millis() - dmrdTxInfo.lastUpdate > 600) {
    dmrdTxInfo.active = false;
  }
  // WiFi reconnect handled by Arduino WiFi library (setAutoReconnect)
  delay(2);
}
