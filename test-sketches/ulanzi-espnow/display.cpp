// display.cpp — Font tables, LED drawing helpers, brightness, auto-rotate,
// and the main display loop.
#include "display.h"
#include "globals.h"
#include "mqtt.h"
#include <AnimatedGIF.h>
#include <TJpg_Decoder.h>
#include <LittleFS.h>

// ============================================================
// 3×5 pixel fonts (file-private)
// ============================================================

static const uint8_t FONT_DIGITS[10][5] = {
  {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
  {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
  {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
  {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
  {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
  {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
  {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
  {0b111, 0b001, 0b010, 0b010, 0b010}, // 7
  {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
  {0b111, 0b101, 0b111, 0b001, 0b111}  // 9
};


// 3×5 font for A–Z
static const uint8_t FONT_ALPHA[26][5] = {
  {0b010,0b101,0b111,0b101,0b101}, // A
  {0b110,0b101,0b110,0b101,0b110}, // B
  {0b011,0b100,0b100,0b100,0b011}, // C
  {0b110,0b101,0b101,0b101,0b110}, // D
  {0b111,0b100,0b110,0b100,0b111}, // E
  {0b111,0b100,0b110,0b100,0b100}, // F
  {0b011,0b100,0b101,0b101,0b011}, // G
  {0b101,0b101,0b111,0b101,0b101}, // H
  {0b111,0b010,0b010,0b010,0b111}, // I
  {0b001,0b001,0b001,0b101,0b010}, // J
  {0b101,0b101,0b110,0b101,0b101}, // K
  {0b100,0b100,0b100,0b100,0b111}, // L
  {0b101,0b111,0b111,0b101,0b101}, // M
  {0b101,0b111,0b101,0b101,0b101}, // N
  {0b010,0b101,0b101,0b101,0b010}, // O
  {0b110,0b101,0b110,0b100,0b100}, // P
  {0b010,0b101,0b101,0b011,0b001}, // Q
  {0b110,0b101,0b110,0b101,0b101}, // R
  {0b011,0b100,0b010,0b001,0b110}, // S
  {0b111,0b010,0b010,0b010,0b010}, // T
  {0b101,0b101,0b101,0b101,0b111}, // U
  {0b101,0b101,0b101,0b101,0b010}, // V
  {0b101,0b101,0b111,0b111,0b101}, // W
  {0b101,0b101,0b010,0b101,0b101}, // X
  {0b101,0b101,0b010,0b010,0b010}, // Y
  {0b111,0b001,0b010,0b100,0b111}, // Z
};


// 3×5 bitmaps for punctuation/symbols found in POCSAG messages
// ~ is used as a degree symbol (°) in the temperature format string
static const char    SPECIAL_CHARS[]   = "-.:/%=+()!?#@~";
static const uint8_t FONT_SPECIAL[][5] = {
  {0b000,0b000,0b111,0b000,0b000}, // -
  {0b000,0b000,0b000,0b000,0b010}, // .
  {0b000,0b010,0b000,0b010,0b000}, // :
  {0b001,0b001,0b010,0b100,0b100}, // /
  {0b101,0b001,0b010,0b100,0b101}, // %
  {0b000,0b111,0b000,0b111,0b000}, // =
  {0b000,0b010,0b111,0b010,0b000}, // +
  {0b001,0b010,0b010,0b010,0b001}, // (
  {0b100,0b010,0b010,0b010,0b100}, // )
  {0b010,0b010,0b010,0b000,0b010}, // !
  {0b111,0b001,0b010,0b000,0b010}, // ?
  {0b010,0b101,0b111,0b101,0b101}, // # (hash/pound)
  {0b010,0b101,0b111,0b100,0b011}, // @
  {0b010,0b101,0b010,0b000,0b000}, // ~ used as ° (degree: small circle at top)
};

// Sentinel returned by drawGifIcon / drawJpegIcon / drawIcon on failure.
// Must be more negative than any valid off-screen x position.
// Worst case: pocsagScrollX reaches -(POCSAG_ICON_RESERVED_PX + POCSAG_MSG_MAX_LEN*4) ≈ -329.
#define ICON_DRAW_FAILED  (-9999)

// ============================================================
// GIF icon rendering (AnimatedGIF + LittleFS) — file-private state
// ============================================================

static AnimatedGIF _gif;
static File        _gifFile;
static int         _gifX0, _gifY0;
static bool        _gifIsOpen   = false;
static char        _gifCurPath[64] = "";

static void* _gifOpen(const char* fname, int32_t* pSize) {
  _gifFile = LittleFS.open(fname);
  if (_gifFile) { *pSize = (int32_t)_gifFile.size(); return &_gifFile; }
  return nullptr;
}
static void _gifClose(void* h) { if (h) ((File*)h)->close(); }
static int32_t _gifRead(GIFFILE* pf, uint8_t* pBuf, int32_t iLen) {
  int32_t n = (int32_t)((File*)pf->fHandle)->read(pBuf, iLen);
  pf->iPos += n;
  return n;
}
static int32_t _gifSeek(GIFFILE* pf, int32_t iPos) {
  if (iPos == 0 && pf->iPos > 0) {
    // Auto-rewind: clear the icon area so frame 0 draws on black (not stale last frame)
#if ESPNOW_DEBUG
    DLOG("[GIF] loop rewind iPos=%d\n", pf->iPos);
#endif
    int cw = _gif.getCanvasWidth();
    int ch = _gif.getCanvasHeight();
    for (int x = 0; x < cw; x++)
      for (int y = _gifY0; y < _gifY0 + ch; y++)
        setLED(_gifX0 + x, y, CRGB::Black);
  }
  ((File*)pf->fHandle)->seek(iPos);
  pf->iPos = iPos;
  return iPos;
}
static void _gifDraw(GIFDRAW* pDraw) {
  int row      = _gifY0 + pDraw->iY + pDraw->y;
  uint8_t*  s  = pDraw->pPixels;
  uint16_t* p  = pDraw->pPalette;
  for (int x = 0; x < pDraw->iWidth; x++) {
    uint8_t idx = s[x];
    if (pDraw->ucHasTransparency && idx == pDraw->ucTransparent) continue;
    uint16_t c  = p[idx];
    uint8_t  r  = ((c >> 11) & 0x1F) << 3;
    uint8_t  g  = ((c >>  5) & 0x3F) << 2;
    uint8_t  b  = ( c        & 0x1F) << 3;
    setLED(_gifX0 + pDraw->iX + x, row, CRGB(r, g, b));
  }
}

// Open GIF for path if not already open; returns true if ready.
static bool _gifEnsureOpen(const char* path) {
  if (_gifIsOpen && strcmp(_gifCurPath, path) == 0) return true;
  if (_gifIsOpen) { _gif.close(); _gifIsOpen = false; }
  _gif.begin(LITTLE_ENDIAN_PIXELS);
  if (!_gif.open(path, _gifOpen, _gifClose, _gifRead, _gifSeek, _gifDraw)) {
    LOG("[GIF] open FAILED: %s\n", path);
    return false;
  }
  DLOG("[GIF] opened %s  canvas=%dx%d\n",
    path, _gif.getCanvasWidth(), _gif.getCanvasHeight());
  strncpy(_gifCurPath, path, sizeof(_gifCurPath) - 1);
  _gifCurPath[sizeof(_gifCurPath) - 1] = '\0';
  _gifIsOpen = true;
  return true;
}

void setupDisplay() {
  _gif.begin(LITTLE_ENDIAN_PIXELS);  // set pixel byte order once at boot
}

// ============================================================
// Indicators — 3 status dots on the right edge (AWTRIX3 style)
// Pixel layout (serpentine 32×8 matrix):
//   Ind1 top:    (31,0) (30,0) (31,1)   — WiFi
//   Ind2 middle: (31,3) (31,4)           — POCSAG 10 s flash
//   Ind3 bottom: (31,6) (31,7) (30,7)   — MQTT
// Called just before every FastLED.show() inside loopDisplay().
// ============================================================

void drawIndicators() {
  if (!indicatorsEnabled) return;
#if RECV_POCSAG
  if (pocsagMsgActive) return;  // full screen for message display dont display indicators
#endif

  // Ind1 — WiFi: green = connected, red blink = disconnected
  static bool          ind1Blink = false;
  static unsigned long ind1Last  = 0;
  if (millis() - ind1Last >= 500) { ind1Blink = !ind1Blink; ind1Last = millis(); }
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  CRGB c1 = wifiOk ? CRGB(0, 180, 0) : (ind1Blink ? CRGB(200, 0, 0) : CRGB::Black);
  setLED(31, 0, c1); setLED(30, 0, c1); setLED(31, 1, c1);

  // Ind2 — POCSAG: 10 s orange blink after message finishes on display
  static bool          ind2Blink = false;
  static unsigned long ind2Last  = 0;
  if (millis() - ind2Last >= 500) { ind2Blink = !ind2Blink; ind2Last = millis(); }
  CRGB c2 = (millis() < pocsagIndicatorUntil)
              ? (ind2Blink ? CRGB(255, 100, 0) : CRGB::Black)
              : CRGB::Black;
  setLED(31, 3, c2); setLED(31, 4, c2);

  // Ind3 — MQTT: cyan = connected, red = disconnected (hidden when disabled)
  CRGB c3 = CRGB::Black;
  if (mqttEnabled)
    c3 = mqttIsConnected() ? CRGB(0, 200, 200) : CRGB(200, 0, 0);
  setLED(31, 6, c3); setLED(31, 7, c3); setLED(30, 7, c3);
}

void _gifCloseIfOpen() {
  if (_gifIsOpen) { _gif.close(); _gifIsOpen = false; _gifCurPath[0] = '\0'; }
}

// ── Screensaver brightness save/restore ──────────────────────────────────────

static uint8_t _ssSavedBrightness = 0;
static bool    _ssSavedAuto       = false;
static bool    _ssBrightApplied   = false;

static void _applySsBrightness() {
  if (_ssBrightApplied) return;
  _ssSavedBrightness = currentBrightness;
  _ssSavedAuto       = autoBrightnessEnabled;
  _ssBrightApplied   = true;
  if (screensaverBrightness == -2) {                 // Auto — follow LDR
    autoBrightnessEnabled = true;
  } else {                                           // Fixed preset (always own value)
    autoBrightnessEnabled = false;
    currentBrightness     = (uint8_t)screensaverBrightness;
    FastLED.setBrightness(currentBrightness);
  }
}

// Called when screensaverBrightness changes while the screensaver is already running.
void screensaverApplyBrightness() {
  if (!screensaverActive || !_ssBrightApplied) return;
  if (screensaverBrightness == -2) {
    autoBrightnessEnabled = true;
  } else {
    autoBrightnessEnabled = false;
    currentBrightness     = (uint8_t)screensaverBrightness;
    FastLED.setBrightness(currentBrightness);
  }
}

static void _restoreSsBrightness() {
  if (!_ssBrightApplied) return;
  _ssBrightApplied      = false;
  autoBrightnessEnabled = _ssSavedAuto;
  currentBrightness     = _ssSavedBrightness;
  if (!autoBrightnessEnabled) FastLED.setBrightness(currentBrightness);
}

void resetScreensaverIdle() {
  if (screensaverActive) { _gifCloseIfOpen(); _restoreSsBrightness(); }
  screensaverActive    = false;
  screensaverIdleStart = millis();
}

// Advance one GIF frame. Keeps the file open for the next call (animation).
// *delayMs is set to the frame delay for the caller's redraw scheduling.
// x0: left edge of icon on the matrix (use 0 for static; pocsagScrollX for scrolling).
// Returns x position for text (= x0 + gifWidth + 1), or ICON_DRAW_FAILED on failure.
static int drawGifIcon(const char* path, int* delayMs, int x0 = 0) {
  *delayMs = 1000;
  if (!fsAvailable) return ICON_DRAW_FAILED;
  if (!_gifEnsureOpen(path)) return ICON_DRAW_FAILED;
  int w = _gif.getCanvasWidth();
  int h = _gif.getCanvasHeight();
  _gifX0 = x0;
  _gifY0 = (MATRIX_HEIGHT - h) / 2;
  int delay = 100;
  int result = _gif.playFrame(false, &delay);
  *delayMs = max(delay, 33);
  if (result < 0) {
    _gif.close();  // decode error: reopen on next call
    _gifIsOpen = false;
    _gifCurPath[0] = '\0';
    return ICON_DRAW_FAILED;          // force bitmap fallback immediately
  }
  if (result == 0) {
    // Last frame of a non-looping GIF — close so next call reopens from frame 0.
    // Last frame was already drawn so we still return a valid text position.
    _gif.close();
    _gifIsOpen = false;
    _gifCurPath[0] = '\0';
  }
  return x0 + w + 1;                 // text starts right after GIF + 1px gap
}

// ============================================================
// JPEG icon rendering (TJpg_Decoder + LittleFS)
// ============================================================

static bool jpgMatrixOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  for (int row = 0; row < h; row++) {
    for (int col = 0; col < w; col++) {
      uint16_t color = bitmap[row * w + col];
      uint8_t r = ((color >> 11) & 0x1F) << 3;
      uint8_t g = ((color >> 5) & 0x3F) << 2;
      uint8_t b = (color & 0x1F) << 3;
      setLED(x + col, y + row, CRGB(r, g, b));
    }
  }
  return true;
}

// Decode a JPEG icon from LittleFS, draw it at x0.
// Returns x for text (= x0 + actualWidth + 1), or ICON_DRAW_FAILED on failure.
static int drawJpegIcon(const char* path, int* delayMs, int x0 = 0) {
  *delayMs = 1000;
  if (!fsAvailable) return ICON_DRAW_FAILED;
  // Get actual dimensions via path+FS (opens/closes internally, safe)
  uint16_t w = 8, h = 8;
  TJpgDec.getFsJpgSize(&w, &h, path, LittleFS);
  h = min((uint16_t)MATRIX_HEIGHT, h);
  File jpgFile = LittleFS.open(path);
  if (!jpgFile) {
    LOG("[JPEG] open FAILED: %s\n", path);
    return ICON_DRAW_FAILED;
  }
  TJpgDec.setCallback(jpgMatrixOutput);
  TJpgDec.setJpgScale(1);
  bool ok = (TJpgDec.drawFsJpg(x0, (MATRIX_HEIGHT - h) / 2, jpgFile) == JDR_OK);
  jpgFile.close();
  if (!ok) return ICON_DRAW_FAILED;
  return x0 + (int)w + 1;
}

static bool _isJpeg(const char* path) {
  const char* dot = strrchr(path, '.');
  if (!dot) return false;
  return strcasecmp(dot, ".jpg") == 0 || strcasecmp(dot, ".jpeg") == 0;
}

// Unified icon draw: routes to GIF or JPEG decoder based on file extension.
// x0: left edge of icon; 0 for static displays, pocsagScrollX for scrolling.
static int drawIcon(const char* path, int* delayMs, int x0 = 0) {
  if (_isJpeg(path)) return drawJpegIcon(path, delayMs, x0);
  return drawGifIcon(path, delayMs, x0);
}

// ============================================================
// LED drawing primitives
// ============================================================

void setLED(int x, int y, CRGB color) {
  if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) return;
  int idx = (y % 2 == 0) ? y * MATRIX_WIDTH + x : (y + 1) * MATRIX_WIDTH - 1 - x;
  leds[idx] = color;
}

static void drawDigit(int x, int y, int d, CRGB color) {
  for (int row = 0; row < 5; row++)
    for (int col = 0; col < 3; col++)
      if (FONT_DIGITS[d][row] & (1 << (2 - col)))
        setLED(x + col, y + row, color);
}

// Draw HH:MM:SS centered in the 32×8 matrix (27px content, 5px digit height)
static void drawTime(int h, int m, int s, CRGB color) {
  const int xo = (MATRIX_WIDTH  - 27 + 1) / 2;  // = 3  (3px left, 2px right)
  const int yo = (MATRIX_HEIGHT -  5)     / 2;  // = 1  (1px top, 2px bottom)
  drawDigit(xo +  0, yo, h / 10, color);
  drawDigit(xo +  4, yo, h % 10, color);
  setLED(xo +  8, yo + 1, color); setLED(xo +  8, yo + 3, color);  // colon
  drawDigit(xo + 10, yo, m / 10, color);
  drawDigit(xo + 14, yo, m % 10, color);
  setLED(xo + 18, yo + 1, color); setLED(xo + 18, yo + 3, color);  // colon
  drawDigit(xo + 20, yo, s / 10, color);
  drawDigit(xo + 24, yo, s % 10, color);
}

static void drawStatusWord(const char* word, CRGB color) {
  int len   = strlen(word);
  int width = len * 4 - 1;
  int xo    = (MATRIX_WIDTH  - width + 1) / 2;
  int yo    = (MATRIX_HEIGHT - 5)         / 2;
  FastLED.clear();
  for (int i = 0; i < len; i++)
    drawChar(xo + i * 4, yo, word[i], color);
  FastLED.show();
}

// drawUpdate: called once on OTA start — sets text + full dim track on row 7
void drawUpdate() {
  const char* word = "UPDATE";
  int len   = 6;
  int xo    = (MATRIX_WIDTH  - (len * 4 - 1) + 1) / 2;
  int yo    = (MATRIX_HEIGHT - 5)                  / 2;
  FastLED.clear();
  for (int i = 0; i < len; i++)
    drawChar(xo + i * 4, yo, word[i], LED_COLOR_TIME);
  for (int x = 0; x < MATRIX_WIDTH; x++)
    setLED(x, 7, CRGB(0, 25, 25));  // dim progress track, row 7
  FastLED.show();
}

// drawProgress: redraws the full bar row on every call — corrects any corruption
void drawProgress(int barW) {
  for (int x = 0; x < MATRIX_WIDTH; x++)
    setLED(x, 7, (x < barW) ? CRGB::Cyan : CRGB(0, 25, 25));
  FastLED.show();
}

void drawDone()   { drawStatusWord("DONE",  CRGB::Green); }
void drawError()  { drawStatusWord("ERROR", CRGB::Red);   }

// Boot screen — device name in rainbow colours, letters appear one by one.
void drawBootScreen() {
  static const CRGB bootColors[] = {
    CRGB(255,   0,   0),  // red
    CRGB(255, 100,   0),  // orange
    CRGB(200, 200,   0),  // yellow
    CRGB(  0, 200,   0),  // green
    CRGB(  0, 160, 255),  // cyan-blue
    CRGB(160,   0, 255),  // violet
  };
  int len = strlen(bootName);
  if (len == 0) return;
  int width = len * 4 - 1;
  const int xo = (MATRIX_WIDTH  - width + 1) / 2;
  const int yo = (MATRIX_HEIGHT -  5)        / 2;
  FastLED.clear();
  for (int i = 0; i < len; i++) {
    drawChar(xo + i * 4, yo, bootName[i], bootColors[i % 6]);
    FastLED.show();
    delay(200);
  }
  delay(1200);  // hold complete word visible
}

void drawChar(int x, int y, char c, CRGB color) {
  if (c >= 'a' && c <= 'z') c -= 32;
  if (c >= '0' && c <= '9') { drawDigit(x, y, c - '0', color); return; }
  if (c >= 'A' && c <= 'Z') {
    const uint8_t* g = FONT_ALPHA[c - 'A'];
    for (int row = 0; row < 5; row++)
      for (int col = 0; col < 3; col++)
        if (g[row] & (1 << (2 - col)))
          setLED(x + col, y + row, color);
    return;
  }
  for (int i = 0; SPECIAL_CHARS[i]; i++) {
    if (SPECIAL_CHARS[i] == c) {
      const uint8_t* g = FONT_SPECIAL[i];
      for (int row = 0; row < 5; row++)
        for (int col = 0; col < 3; col++)
          if (g[row] & (1 << (2 - col)))
            setLED(x + col, y + row, color);
      return;
    }
  }
  // space and other unknowns render as a blank gap (no pixels set)
}

// ============================================================
// Brightness — LDR auto-dim
// ============================================================

void loopBrightness() {
  static bool prevAuto = false;
  if (!autoBrightnessEnabled) { prevAuto = false; return; }

  static unsigned long lastUpdate = 0;
  bool justEnabled = !prevAuto;
  prevAuto = true;

  if (!justEnabled && millis() - lastUpdate < LDR_UPDATE_MS) return;
  lastUpdate = millis();

  int ldr = constrain(analogRead(LDR_PIN), LDR_ADC_DARK, LDR_ADC_BRIGHT);
  uint8_t target = (uint8_t)map(ldr, LDR_ADC_DARK, LDR_ADC_BRIGHT, LDR_MIN_BRIGHTNESS, 255);
  // Snap immediately on first auto-enable; EMA smoothing for subsequent updates
  currentBrightness = justEnabled ? target : (uint8_t)((currentBrightness * 3 + target + 2) / 4);
  FastLED.setBrightness(currentBrightness);
}

// ============================================================
// Auto-rotation — cycles clock→temp→humidity on a timer
// ============================================================

void loopAutoRotate() {
  if (!autoRotateEnabled) return;
  if (screensaverActive || iconPreviewActive) return;

  static unsigned long lastRotate = 0;

#if RECV_POCSAG
  static bool        prevPocsag   = false;
  static DisplayMode savedMode    = MODE_CLOCK;
  if (pocsagMsgActive) {                               // pause during message
    if (!prevPocsag) savedMode = displayMode;          // snapshot mode on first pause frame
    prevPocsag = true;
    return;
  }
  if (prevPocsag) {                                    // message just ended
    prevPocsag  = false;
    displayMode = savedMode;                           // restore pre-message mode
    lastRotate  = millis();   // restart rotation timer from now
    return;
  }
#endif

  if (millis() - lastRotate < (unsigned long)autoRotateIntervalSec * 1000) return;
  lastRotate = millis();
  // Advance to next mode; skip sensor modes if SHT31 not available
  do {
    displayMode = (DisplayMode)((displayMode + 1) % MODE_COUNT);
  } while (!sht31Available && (displayMode == MODE_TEMP || displayMode == MODE_HUMIDITY));
}

// ============================================================
// Dynamic color helpers — map sensor values to zone colors
// ============================================================

static CRGB colorForTemp(float t) {
  if (t < tempThreshLo) return colorTempLo;
  if (t < tempThreshHi) return colorTempMid;
  return colorTempHi;
}

static CRGB colorForHum(float h) {
  if (h < humThreshLo) return colorHumLo;
  if (h < humThreshHi) return colorHumMid;
  return colorHumHi;
}

static CRGB colorForBat(int pct) {
  if (pct <= (int)batThreshLo) return colorBatLo;
  if (pct <= (int)batThreshHi) return colorBatMid;
  return colorBatHi;
}

// ============================================================
// Main display loop
// ============================================================

const char* getScreenName() {
#if RECV_POCSAG
  if (pocsagMsgActive)   return "POCSAG";
#endif
  if (screensaverActive) return "screensaver";
  switch (displayMode) {
    case MODE_CLOCK:    return "clock";
    case MODE_TEMP:     return "temp";
    case MODE_HUMIDITY: return "humidity";
    case MODE_BATTERY:  return "battery";
    default:            return "?";
  }
}

void loopDisplay() {
  if (otaInProgress) return;  // OTA owns the display — don't touch it

  // Log screen transitions
  {
    static const char* prevScreen = "";
    const char* cur = getScreenName();
    if (cur != prevScreen) {
      LOG("[DISP] Screen: %s\n", cur);
      prevScreen = cur;
      mqttNotifyState();
    }
  }

  // POCSAG message display — takes priority over all modes
#if RECV_POCSAG
  if (pocsagMsgActive) {
    const int yo = (MATRIX_HEIGHT - 5) / 2;
    if (!pocsagIsScrolling) {
      // Static: redraw at GIF frame rate (animated icon) or 500 ms for bitmaps
      static int _pocsagStaticGifDelay = 100;
      if (pocsagStaticLastDraw == 0 ||
          millis() - pocsagStaticLastDraw >= (unsigned long)_pocsagStaticGifDelay) {
        bool first = (pocsagStaticLastDraw == 0);
        if (first) _pocsagStaticGifDelay = 100;  // reset for each new message
        FastLED.clear();
        int gifDelay = 500;
        int textX = drawIcon(iconPocsagFile, &gifDelay);
        if (textX == ICON_DRAW_FAILED) {
          gifDelay = 500;
          textX = 0;
        }
        _pocsagStaticGifDelay = max(gifDelay, 50);
        // Center text in the space to the right of the icon
        int textW = pocsagMsgLen * 4 - 1;
        int availW = MATRIX_WIDTH - textX;
        int xo = textX + max(0, (availW - textW) / 2);
        for (int i = 0; i < pocsagMsgLen; i++)
          drawChar(xo + i * 4, yo, pocsagMsg[i], colorPocsag);
        drawIndicators();
        FastLED.show();
        pocsagStaticLastDraw = millis();
        if (first) LOG("[DISP] POCSAG '%s'\n", pocsagMsg);
      }
      if (millis() >= pocsagStaticUntil) {
        pocsagMsgActive       = false;
        screensaverIdleStart  = millis();  // restart idle countdown after message
        pocsagIndicatorUntil  = millis() + 10000;  // blink Ind2 for 10 s after display ends
      }
    } else {
      // Scroll: icon pinned at x=0 (if present), text scrolls across matrix
      if (millis() - pocsagScrollLast < POCSAG_SCROLL_SPEED_MS) return;
      pocsagScrollLast = millis();

      static bool          _scrollFirst     = true;
      static unsigned long _scrollNextGifMs = 0;
      static bool          _scrollHasIcon   = false;
      if (_scrollFirst) {
        _scrollNextGifMs = 0;  // force icon probe on first frame
        _scrollFirst = false;
      }

      // Advance icon frame only when GIF delay has elapsed; track whether icon is present
      if (millis() >= _scrollNextGifMs) {
        int gifDelay = 100;
        int textX = drawIcon(iconPocsagFile, &gifDelay, 0);
        _scrollHasIcon = (textX != ICON_DRAW_FAILED);
        _scrollNextGifMs = millis() + max(gifDelay, 33);
      }

      // Clear text area; preserve icon area only when icon is present
      int clearFrom = _scrollHasIcon ? POCSAG_ICON_RESERVED_PX - 1 : 0;
      for (int x = clearFrom; x < MATRIX_WIDTH; x++)
        for (int y = 0; y < MATRIX_HEIGHT; y++)
          setLED(x, y, CRGB::Black);

      // Draw scrolling text; clip behind icon if present, full width if not
      int clipFrom = _scrollHasIcon ? POCSAG_ICON_RESERVED_PX : 0;
      for (int i = 0; i < pocsagMsgLen; i++) {
        int cx = pocsagScrollX + i * 4;
        if (cx >= clipFrom)
          drawChar(cx, yo, pocsagMsg[i], colorPocsag);
      }

      drawIndicators();
      FastLED.show();
      pocsagScrollX--;
      if (pocsagScrollX < -(pocsagMsgLen * 4)) {
        if (++pocsagScrollPass >= POCSAG_SCROLL_PASSES) {
          pocsagMsgActive      = false;
          screensaverIdleStart = millis();
          pocsagIndicatorUntil = millis() + 10000;  // blink Ind2 for 10 s after scroll ends
          _scrollFirst         = true;   // reset for next message
        } else {
          pocsagScrollX = MATRIX_WIDTH;
        }
      }
    }
    return;
  }
#endif

  // IP address scroll — shown once after WiFi connects (2 passes)
  if (ipScrollActive) {
    if (millis() - ipScrollLast < POCSAG_SCROLL_SPEED_MS) return;
    ipScrollLast = millis();
    const int yo = (MATRIX_HEIGHT - 5) / 2;
    FastLED.clear();
    for (int i = 0; i < ipScrollLen; i++)
      drawChar(ipScrollX + i * 4, yo, ipScrollMsg[i], CRGB(0, 220, 120));
    drawIndicators();
    FastLED.show();
    ipScrollX--;
    if (ipScrollX < -(ipScrollLen * 4)) {
      if (++ipScrollPass >= 2) {
        ipScrollActive = false;
        displayMode    = MODE_CLOCK;  // always return to clock after IP scroll
      } else {
        ipScrollX = MATRIX_WIDTH;
      }
    }
    return;
  }

  // Icon preview — show selected icon on display for 3s (triggered by Show button)
  if (iconPreviewActive) {
    static unsigned long nextPreviewFrame  = 0;
    static bool          prevPreviewActive = false;
    if (millis() >= iconPreviewUntil) {
      iconPreviewActive = false;
      prevPreviewActive = false;
      _gifCloseIfOpen();
      FastLED.clear();
      FastLED.show();
    } else {
      if (!prevPreviewActive) nextPreviewFrame = 0;  // reset on each new preview
      prevPreviewActive = true;
      if (millis() >= nextPreviewFrame) {
        FastLED.clear();
        int gifDelay = 500;
        drawIcon(iconPreviewFile, &gifDelay, 0);
        drawIndicators();
        FastLED.show();
        nextPreviewFrame = millis() + max(gifDelay, 33);
      }
    }
    return;
  }

  // Screensaver — render if active (test or auto), auto-activate only when enabled
  if (screensaverActive) {
    static unsigned long nextSsFrame = 0;
    if (millis() >= nextSsFrame) {
      if (_gifEnsureOpen(screensaverFile)) {
        _gifX0 = 0;
        _gifY0 = 0;
        int delay = 100;
        int r = _gif.playFrame(false, &delay);
        FastLED.show();
        nextSsFrame = millis() + max(delay, 33);
        if (r < 0) { _gif.close(); _gifIsOpen = false; _gifCurPath[0] = '\0'; screensaverActive = false; _restoreSsBrightness(); }
      } else {
        screensaverActive = false;  // file missing/corrupt — abort
        _restoreSsBrightness();
      }
    }
    return;
  }
  if (screensaverEnabled && strlen(screensaverFile) > 0) {
    if (millis() - screensaverIdleStart >= (unsigned long)screensaverTimeoutSec * 1000) {
      screensaverActive = true;
      _gifCloseIfOpen();
      _applySsBrightness();
      FastLED.clear();   // clear display once before first frame draws
      FastLED.show();
      LOG("[SS] Screensaver activated (bright=%d)\n", screensaverBrightness);
      return;
    }
  }

  // Auto-return to clock after mode timeout (manual presses only; rotation manages itself)
  if (!autoRotateEnabled && displayMode != MODE_CLOCK && millis() >= modeActiveUntil)
    displayMode = MODE_CLOCK;

  // Scanner animation while waiting for first time sync (clock mode only)
  if (!timeSynced && displayMode == MODE_CLOCK) {
    static unsigned long lastScan = 0;
    if (millis() - lastScan < 40) return;
    lastScan = millis();

    static int scanPos = 0;
    static int scanDir = 1;

    FastLED.clear();
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      int dist = abs(x - scanPos);
      uint8_t bright = (dist == 0) ? 200 : (dist == 1) ? 80 : (dist == 2) ? 30 : (dist == 3) ? 10 : 0;
      if (bright > 0) {
        CRGB col(0, bright / 4, bright);  // blue-ish
        for (int y = 0; y < MATRIX_HEIGHT; y++) setLED(x, y, col);
      }
    }
    drawIndicators();
    FastLED.show();

    scanPos += scanDir;
    if (scanPos >= MATRIX_WIDTH - 1 || scanPos <= 0) scanDir = -scanDir;
    return;
  }

  // Close GIF when not in a mode that uses it
  if (!screensaverActive &&
      displayMode != MODE_TEMP && displayMode != MODE_HUMIDITY && displayMode != MODE_BATTERY)
    _gifCloseIfOpen();

  // Temperature / humidity display
  if (displayMode == MODE_TEMP || displayMode == MODE_HUMIDITY) {
    static DisplayMode   lastMode = MODE_CLOCK;
    static unsigned long nextDraw = 0;
    bool modeChanged = (lastMode != displayMode);
    lastMode = displayMode;

    if (modeChanged) { _gifCloseIfOpen(); nextDraw = 0; FastLED.clear(); }
    if (millis() < nextDraw) return;

    const int yo = (MATRIX_HEIGHT - 5) / 2;
    char buf[10];
    CRGB color;
    int gifDelay = 1000;

    if (displayMode == MODE_TEMP) {
      int t100 = (int)roundf(sht31Temp * 100.0f);
      if (t100 >= 0)
        snprintf(buf, sizeof(buf), "%d.%02d~", t100 / 100, t100 % 100);
      else
        snprintf(buf, sizeof(buf), "-%d.%02d~", (-t100) / 100, (-t100) % 100);
      color = colorForTemp(sht31Temp);

      int len   = strlen(buf);
      int textW = len * 4 - 1;
      int textX = drawIcon(iconTempFile, &gifDelay);
      if (textX == ICON_DRAW_FAILED) {
        FastLED.clear();
        gifDelay = 1000;
        // No icon: center text across full matrix
        textX = (MATRIX_WIDTH - textW + 1) / 2;
      } else {
        // Icon present: clear text area, center text in remaining space
        for (int x = textX - 1; x < MATRIX_WIDTH; x++)
          for (int y = 0; y < MATRIX_HEIGHT; y++)
            setLED(x, y, CRGB::Black);
        textX = textX + (MATRIX_WIDTH - textX - textW) / 2;
      }
      for (int i = 0; i < len; i++)
        drawChar(textX + i * 4, yo, buf[i], color);

    } else {
      int h10 = constrain((int)roundf(sht31Hum * 10.0f), 0, 1000);
      snprintf(buf, sizeof(buf), "%d.%d%%", h10 / 10, h10 % 10);
      color = colorForHum(sht31Hum);

      int len   = strlen(buf);
      int textW = len * 4 - 1;
      int textX = drawIcon(iconHumFile, &gifDelay);
      if (textX == ICON_DRAW_FAILED) {
        FastLED.clear();
        gifDelay = 1000;
        // No icon: center text across full matrix
        textX = (MATRIX_WIDTH - textW + 1) / 2;
      } else {
        // Icon present: clear text area, center text in remaining space
        for (int x = textX - 1; x < MATRIX_WIDTH; x++)
          for (int y = 0; y < MATRIX_HEIGHT; y++)
            setLED(x, y, CRGB::Black);
        textX = textX + (MATRIX_WIDTH - textX - textW) / 2;
      }
      for (int i = 0; i < len; i++)
        drawChar(textX + i * 4, yo, buf[i], color);
    }

    drawIndicators();
    FastLED.show();
    nextDraw = millis() + gifDelay;
    return;
  }

  // Battery display
  if (displayMode == MODE_BATTERY) {
    static DisplayMode   lastBatMode = MODE_CLOCK;
    static unsigned long nextBatDraw = 0;
    bool batChanged = (lastBatMode != displayMode);
    lastBatMode = displayMode;

    if (batChanged) { _gifCloseIfOpen(); nextBatDraw = 0; FastLED.clear(); }
    if (millis() < nextBatDraw) return;

    int batRaw = analogRead(BAT_PIN);
    int batPct = (int)constrain(map(batRaw, BAT_RAW_EMPTY, BAT_RAW_FULL, 0, 100), 0, 100);
    CRGB color = colorForBat(batPct);
    const int yo = (MATRIX_HEIGHT - 5) / 2;

    char buf[5];
    snprintf(buf, sizeof(buf), "%d%%", batPct);
    int len   = strlen(buf);
    int textW = len * 4 - 1;
    int gifDelay = 2000;

    int textX = drawIcon(iconBatFile, &gifDelay);
    if (textX == ICON_DRAW_FAILED) {
      FastLED.clear();
      gifDelay = 2000;
      // No icon: center text across full matrix
      textX = (MATRIX_WIDTH - textW + 1) / 2;
    } else {
      // Icon present: clear text area, center text in remaining space
      for (int x = textX - 1; x < MATRIX_WIDTH; x++)
        for (int y = 0; y < MATRIX_HEIGHT; y++)
          setLED(x, y, CRGB::Black);
      textX = textX + (MATRIX_WIDTH - textX - textW) / 2;
    }
    for (int i = 0; i < len; i++)
      drawChar(textX + i * 4, yo, buf[i], color);
    drawIndicators();
    FastLED.show();
    nextBatDraw = millis() + gifDelay;
    return;
  }

  // Clock — update once per second
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 1000) return;
  lastUpdate = millis();

  struct tm t;
  if (!getLocalTime(&t)) return;

  FastLED.clear();
  drawTime(t.tm_hour, t.tm_min, t.tm_sec, colorClock);
  drawIndicators();
  FastLED.show();
}
