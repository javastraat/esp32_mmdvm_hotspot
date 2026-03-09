#ifndef CONFIG_H
#define CONFIG_H

// ─── WiFi (hardcoded — only way to reach BrandMeister / DAPNET) ───────────
#define WIFI_SSID       "TechInc"
#define WIFI_PASSWORD   "itoldyoualready"

// ─── Version ─────────────────────────────────────────────────────────────
#define FW_VERSION  "1.0.0"

// ─── BrandMeister defaults ────────────────────────────────────────────────
#define DEF_BM_ENABLED      false
#define DEF_BM_SERVER       "api.brandmeister.network"
#define DEF_BM_PORT         62031
#define DEF_BM_DMR_ID       0
#define DEF_BM_PASSWORD     "passw0rd"
#define DEF_BM_CALLSIGN     "N0CALL"
#define DEF_BM_SSID         0           // callsign SSID suffix 0-99
#define DEF_BM_LOCATION     "My City"
#define DEF_BM_LATITUDE     "0.000000"
#define DEF_BM_LONGITUDE    "0.000000"
#define DEF_BM_HEIGHT       0

// ─── DAPNET defaults ──────────────────────────────────────────────────────
#define DEF_DAPNET_ENABLED  false
#define DEF_DAPNET_SERVER   "dapnet.afu.rwth-aachen.de"
#define DEF_DAPNET_PORT     43434
#define DEF_DAPNET_CALLSIGN ""          // empty = use BM callsign
#define DEF_DAPNET_AUTHKEY  ""
#define DEF_DAPNET_DEBUG    false

// ─── ESP-NOW defaults ─────────────────────────────────────────────────────
#define DEF_ESPNOW_MACS     ""          // comma-separated MAC(s)

// ─── Limits ──────────────────────────────────────────────────────────────
#define ESPNOW_MAX_PEERS    6
#define POCSAG_MSG_MAX      80
#define LOG_LINES           40

// ─── ESP-NOW packet types (must match system_espnow.h in main firmware) ──
#define ESPNOW_TYPE_DMR_NET  0x10
#define ESPNOW_TYPE_POCSAG   0x11

#endif // CONFIG_H
