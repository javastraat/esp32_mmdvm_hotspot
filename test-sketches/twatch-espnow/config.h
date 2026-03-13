// ── twatch-espnow default configuration ──────────────────────────────────────
// Edit these values and re-flash to change factory defaults.
// At runtime, values are stored in NVS and can be edited from the web UI.
#pragma once

// ── WiFi ──────────────────────────────────────────────────────────────────────
#define DEFAULT_WIFI_SSID   "TechInc"
#define DEFAULT_WIFI_PASS   "itoldyoualready"
#define DEFAULT_HOSTNAME    "twatch-espnow"

// ── POCSAG ────────────────────────────────────────────────────────────────────
#define DEFAULT_TIME_RIC    224     // RIC carrying date/time sync packets

// Hidden RICs — silently processed but never shown on the POCSAG screen.
// Comma-separated list, no spaces.
#define DEFAULT_HIDDEN_RICS "224,208,200,216"
#define MAX_HIDDEN_RICS     16
