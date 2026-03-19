#pragma once
 
// ============================================================
// Protocol modes — enable one or both (at least one required)
// ============================================================
#define RECV_DMR      false  // receive raw DMRD Homebrew packets via ESP-NOW
#define RECV_POCSAG   true   // receive POCSAG pages via ESP-NOW
#define RECV_ESPNOW2  false  // ESP-NOW v2 protocol — coming soon


// ============================================================
// WiFi — connect to the same router as the MMDVM hotspot
// (ensures ESP-NOW uses the correct channel)
// Leave SSID empty "" to skip WiFi and use SoftAP only.
// ============================================================
#define WIFI_SSID     "TechInc"
#define WIFI_PASSWORD "itoldyoualready"


// ============================================================
// Hostname & OTA
// ============================================================
// Default mDNS + ArduinoOTA hostname. Overridable at runtime via the
// Settings page — the saved value is stored in NVS key "mdns_name".
#define MDNS_HOSTNAME "ulanzi-espnow"
#define OTA_PASSWORD  "ulanzi"           // leave empty "" to disable password


// ============================================================
// Debug output — verbose DMR frame logging over Serial
// ============================================================
#define ESPNOW_DEBUG false


// ============================================================
// Time beacon
// POCSAG RIC that carries "YYYYMMDDHHMMSS<YYMMDDHHmmSS>" messages
// ============================================================
#define TIME_POCSAG_RIC  224


// ============================================================
// Transmitter callsign RIC
// RIC that carries the transmitter ID/callsign.
// Trailing digits are stripped before display.
// ============================================================
#define CALLSIGN_RIC  8


// ============================================================
// POCSAG display settings
// ============================================================
// RICs never shown on the LED matrix display (runtime, editable via web).
// Default value used on first boot or after factory reset.
#define EXCLUDED_RICS_DEFAULT "224,208,200,216,4520,4521"

// Number of POCSAG messages kept in the web status log (newest first).
#define POCSAG_LOG_SIZE          10

// Reserved pixel columns for the POCSAG icon (icon width + 1 px gap).
// Used by the scroll-threshold calculation and the display renderer.
#define POCSAG_ICON_RESERVED_PX  9   // 8 px icon + 1 px gap

// Short messages (fits on screen): hold this long, then return to clock.
#define POCSAG_STATIC_MS      15000   // ms to show a short message

// Long messages (scrolling): scroll speed and number of passes.
#define POCSAG_SCROLL_SPEED_MS   50   // ms per pixel scroll step
#define POCSAG_SCROLL_PASSES      3   // how many times to scroll through


// ============================================================
// Battery voltage monitoring (GPIO34 ADC)
// Calibration source: PixelIt TC001 firmware (empirical, real hardware)
// The TC001 uses a large voltage divider — raw ADC range is 510–660,
// NOT what a simple 1:2 divider formula would predict.
// To recalibrate: watch battery_raw in /api/status
//   fully charged → BAT_RAW_FULL, fully depleted → BAT_RAW_EMPTY
// ============================================================
#define BAT_PIN        34      // ADC6 — wired to battery via voltage divider
#define BAT_RAW_EMPTY  510     // ADC raw at 0% (depleted, ~3.0 V)
#define BAT_RAW_FULL   660     // ADC raw at 100% (fully charged, ~4.2 V)
#define BAT_FULL_MV    4200    // mV at 100% (for display estimate)
#define BAT_EMPTY_MV   3000    // mV at 0% (for display estimate)


// ============================================================
// LDR auto-brightness (GL5516 on GPIO35)
// ============================================================
#define LDR_PIN            35   // ADC pin
#define LDR_MIN_BRIGHTNESS  5   // floor brightness in auto mode (0–255)
#define LDR_UPDATE_MS    2000   // sample interval
// GL5516 wiring: bright room → higher ADC value → higher brightness.
// Calibrate by watching ldr_raw in /api/status:
//   LDR_ADC_DARK   = value when covering the sensor with your finger
//   LDR_ADC_BRIGHT = value in your brightest normal lighting condition
#define LDR_ADC_DARK    1600
#define LDR_ADC_BRIGHT  4000


// ============================================================
// Buttons (GPIO26=Left, GPIO27=Middle, GPIO14=Right)
// All wired INPUT_PULLUP — active LOW.
// ============================================================
#define BTN_LEFT    26
#define BTN_MIDDLE  27
#define BTN_RIGHT   14
#define BTN_DEBOUNCE_MS  50   // debounce window in ms


// ============================================================
// RTC (DS1307, I2C)
// Default ESP32 I2C pins — change if your wiring differs.
// ============================================================
#define RTC_SDA_PIN  21
#define RTC_SCL_PIN  22


// ============================================================
// Buzzer (passive, GPIO15)
// GPIO15 is held INPUT_PULLDOWN at boot to silence power-on noise;
// setupBuzzer() reconfigures it as LEDC output for tone generation.
// ============================================================
#define BUZZER_PIN            15
#define BUZZER_LEDC_RES        8     // 8-bit duty resolution (0–255)
#define BUZZER_FREQ_BEEP    1000     // Hz — boot & POCSAG beep pitch
#define BUZZER_DUR_BEEP_MS    80     // ms — beep duration
#define BUZZER_FREQ_CLICK   3000     // Hz — button click pitch
#define BUZZER_DUR_CLICK_MS   12     // ms — click duration
// Default volumes (0–255); internally mapped to LEDC duty 0–100
#define BUZZER_VOL_BOOT       80
#define BUZZER_VOL_POCSAG     80
#define BUZZER_VOL_CLICK      60


// ============================================================
// LED matrix display
// ============================================================
#define LED_DATA_PIN    32
#define LED_BRIGHTNESS  50    // 0–255 (also used as initial manual brightness)
#define LED_COLOR_TIME   CRGB::White
#define LED_COLOR_POCSAG CRGB(255, 160, 0)  // amber for pager messages
