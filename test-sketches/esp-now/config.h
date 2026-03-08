#pragma once

// ============================================================
// STEP 1 — Select the role for this device
//           Uncomment exactly ONE line
// ============================================================
//#define ROLE_SENDER
#define ROLE_RECEIVER


// ============================================================
// STEP 2 — SENDER ONLY settings
//           (ignored completely on the receiver)
// ============================================================
#ifdef ROLE_SENDER

  // WiFi network the sender connects to for internet/BrandMeister.
  // ESP-NOW keeps working even when WiFi is down or unreachable.
  #define WIFI_SSID     "TechInc"
  #define WIFI_PASSWORD "itoldyoualready"

  // MAC address of the RECEIVER device.
  // How to find it:
  //   1. Flash the receiver with ROLE_RECEIVER
  //   2. Open Serial monitor (115200 baud)
  //   3. Copy the MAC printed on boot and paste it here
  //#define RECEIVER_MAC { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  #define RECEIVER_MAC { 0xE4, 0xB3, 0x23, 0xF1, 0x42, 0xB4 }
  // How often to send a test packet (milliseconds)
  #define SEND_INTERVAL_MS 1000

#endif  // ROLE_SENDER


// ============================================================
// STEP 2 — RECEIVER settings
//           (no WiFi credentials needed — receiver does not
//            connect to any WiFi network)
// ============================================================
#ifdef ROLE_RECEIVER

  // Debug output:
  //   false = quiet mode — prints one line per new call (srcId/dstId change)
  //           and a summary when the call ends (frame count, duration)
  //   true  = verbose mode — prints every individual DMRD frame
  #define ESPNOW_DEBUG false

#endif  // ROLE_RECEIVER
