#pragma once

// ============================================================
// STEP 1 — Select the role for this device
//           Uncomment exactly ONE line
// ============================================================
//#define ROLE_SENDER
#define ROLE_RECEIVER


// ============================================================
// STEP 2 — Enable test modes (one or both)
// ============================================================
#define TEST_DMR    false    // DMR: forward/receive raw DMRD Homebrew packets
#define TEST_POCSAG true    // POCSAG: forward/receive POCSAG pages


// ============================================================
// STEP 3 — SENDER settings (ignored on receiver)
// ============================================================
#ifdef ROLE_SENDER

  // WiFi network — ESP-NOW works even without an internet connection
  #define WIFI_SSID     "TechInc"
  #define WIFI_PASSWORD "itoldyoualready"

  // MAC address of the receiver device.
  // Flash the receiver, open Serial (115200), copy the MAC printed on boot.
  #define RECEIVER_MAC { 0xE4, 0xB3, 0x23, 0xF1, 0x42, 0xB4 }

  // ── DMR test settings ──────────────────────────────────────
  #define DMR_SEND_INTERVAL_MS  2000      // send a fake DMRD packet every 2 s

  // ── POCSAG test settings ───────────────────────────────────
  #define POCSAG_RIC           2041152    // pager RIC to address
  #define POCSAG_CALLSIGN      "PD2EMC"  // message text
  #define POCSAG_INTERVAL_MS   30000     // send interval (30 s)

  // ── NTP time sync ───────────────────────────────────────────
  #define NTP_SERVER           "pool.ntp.org"
  #define NTP_GMT_OFFSET_SEC   3600      // UTC+1 (CET); use 7200 for UTC+2 (CEST)
  #define NTP_DST_OFFSET_SEC   0

  // ── OTA ─────────────────────────────────────────────────────
  #define OTA_HOSTNAME  "ulanzi-sender"
  #define OTA_PASSWORD  "mmdvm"               // leave empty to disable password

#endif  // ROLE_SENDER


// ============================================================
// STEP 3 — RECEIVER settings
// ============================================================
#ifdef ROLE_RECEIVER

  // ── WiFi (optional but recommended) ────────────────────────
  // Connect to the same router as the sender so ESP-NOW uses
  // the correct channel. Leave SSID empty to skip and go
  // straight to SoftAP mode.
  #define WIFI_SSID     "TechInc"
  #define WIFI_PASSWORD "itoldyoualready"

  // ── OTA ─────────────────────────────────────────────────────
  #define OTA_HOSTNAME  "ulanzi-clock"
  #define OTA_PASSWORD  "mmdvm"               // leave empty to disable password

  // ── Debug output (applies to DMR frames) ───────────────────
  #define ESPNOW_DEBUG false

  // ── Time beacon RIC ─────────────────────────────────────────
  // POCSAG RIC that carries "YYYYMMDDHHMMSS<YYMMDDHHmmSS>" messages
  #define TIME_POCSAG_RIC  224

  // ── Transmitter callsign RIC ─────────────────────────────────
  // RIC that carries the transmitter ID/callsign.
  // Trailing digits are stripped before display (maintainer sometimes appends "1" etc.)
  #define CALLSIGN_RIC  8

  // ── Display exclusion list ───────────────────────────────────
  // RICs that are never shown on the LED matrix display.
  // Add/remove entries as needed.
  #define POCSAG_DISPLAY_EXCLUDED_RICS { 224, 208, 200, 216, 4520, 4521 }

  // ── POCSAG display behaviour ─────────────────────────────────
  // Short messages (fits on screen): hold this long, then return to clock.
  #define POCSAG_STATIC_MS      15000   // ms to show a short message
  // Long messages (scrolling): scroll speed and number of passes.
  #define POCSAG_SCROLL_SPEED_MS   50   // ms per pixel scroll step
  #define POCSAG_SCROLL_PASSES      3   // how many times to scroll through

#endif  // ROLE_RECEIVER


// ============================================================
// LDR auto-brightness (GL5516 on GPIO35)
// ============================================================
#define LDR_PIN            35   // ADC pin
#define LDR_MIN_BRIGHTNESS  5   // floor brightness in auto mode (0–255)
#define LDR_UPDATE_MS    2000   // sample interval
// GL5516 wiring: bright room → higher ADC value → higher brightness.

// ============================================================
// LED matrix display
// ============================================================
#define LED_DATA_PIN    32
#define LED_BRIGHTNESS  50    // 0–255 (also used as initial manual brightness)
#define LED_COLOR_TIME   CRGB::White
#define LED_COLOR_POCSAG CRGB(255, 160, 0)  // amber for pager messages
