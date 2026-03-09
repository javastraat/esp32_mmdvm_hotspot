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

#endif  // ROLE_SENDER


// ============================================================
// STEP 3 — RECEIVER settings
// ============================================================
#ifdef ROLE_RECEIVER

  // Debug output (applies to DMR frames):
  //   false = quiet — prints one line per new call start/end
  //   true  = verbose — prints every individual DMRD frame
  #define ESPNOW_DEBUG false

#endif  // ROLE_RECEIVER
