// display.cpp — Font tables, LED drawing helpers, brightness, auto-rotate,
// and the main display loop.
#include "display.h"
#include "globals.h"
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

// 3×5 bitmaps for the letters in "UPDATE"
static const uint8_t FONT_UPDATE[6][5] = {
  {0b101, 0b101, 0b101, 0b101, 0b111}, // U
  {0b111, 0b101, 0b111, 0b100, 0b100}, // P
  {0b110, 0b101, 0b101, 0b101, 0b110}, // D
  {0b111, 0b101, 0b111, 0b101, 0b101}, // A
  {0b111, 0b010, 0b010, 0b010, 0b010}, // T
  {0b111, 0b100, 0b111, 0b100, 0b111}, // E
};

// Additional 3×5 bitmaps for "DONE" and "ERR"
static const uint8_t FONT_DONE[4][5] = {
  {0b110, 0b101, 0b101, 0b101, 0b110}, // D
  {0b111, 0b101, 0b101, 0b101, 0b111}, // O
  {0b101, 0b111, 0b101, 0b101, 0b101}, // N
  {0b111, 0b100, 0b111, 0b100, 0b111}, // E
};

static const uint8_t FONT_ERROR[5][5] = {
  {0b111, 0b100, 0b111, 0b100, 0b111}, // E
  {0b111, 0b101, 0b111, 0b110, 0b101}, // R
  {0b111, 0b101, 0b111, 0b110, 0b101}, // R
  {0b111, 0b101, 0b101, 0b101, 0b111}, // O
  {0b111, 0b101, 0b111, 0b110, 0b101}, // R
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

// 3×5 thermometer icon (temperature)
static const uint8_t ICON_THERMO[5] = {0b010, 0b010, 0b011, 0b111, 0b111};

// 5×5 bell icon — POCSAG message fallback
static const uint8_t ICON_MSG[5] = {
  0b00100,  // stem
  0b01110,  // bell top
  0b01110,  // bell body
  0b11111,  // bell base
  0b00100,  // clapper
};

// 5×8 water drop icon, full matrix height (humidity)
// bit 4 = leftmost column, bit 0 = rightmost column
static const uint8_t ICON_DROP[8] = {
  0b00100,  // row 0 — tip
  0b00100,  // row 1 — tip
  0b01110,  // row 2 — shoulder
  0b11111,  // row 3 — body
  0b11111,  // row 4 — body
  0b11111,  // row 5 — body
  0b11111,  // row 6 — body
  0b01110,  // row 7 — rounded base
};

// 6×5 battery outline icon (battery level); interior cols 1–3, rows 1–3 filled by level
static const uint8_t ICON_BAT[5] = {
  0b111110,  // row 0 — top border
  0b100011,  // row 1 — left border + right terminal bump
  0b100011,  // row 2
  0b100011,  // row 3
  0b111110,  // row 4 — bottom border
};

// 3×5 bitmaps for punctuation/symbols found in POCSAG weather messages
static const char    SPECIAL_CHARS[]   = "-.:/%=";
static const uint8_t FONT_SPECIAL[][5] = {
  {0b000,0b000,0b111,0b000,0b000}, // -
  {0b000,0b000,0b000,0b000,0b010}, // .
  {0b000,0b010,0b000,0b010,0b000}, // :
  {0b001,0b001,0b010,0b100,0b100}, // /
  {0b101,0b001,0b010,0b100,0b101}, // %
  {0b000,0b111,0b000,0b111,0b000}, // =
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
    Serial.printf("[GIF] loop rewind iPos=%d\n", pf->iPos);
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
  _gif.begin(LITTLE_ENDIAN_PIXELS);   // palette in native ESP32 byte order → correct RGB
  if (!_gif.open(path, _gifOpen, _gifClose, _gifRead, _gifSeek, _gifDraw)) {
    Serial.printf("[GIF] open FAILED: %s\n", path);
    return false;
  }
  Serial.printf("[GIF] opened %s  canvas=%dx%d\n",
    path, _gif.getCanvasWidth(), _gif.getCanvasHeight());
  strncpy(_gifCurPath, path, sizeof(_gifCurPath) - 1);
  _gifCurPath[sizeof(_gifCurPath) - 1] = '\0';
  _gifIsOpen = true;
  return true;
}

void _gifCloseIfOpen() {
  if (_gifIsOpen) { _gif.close(); _gifIsOpen = false; _gifCurPath[0] = '\0'; }
}

void resetScreensaverIdle() {
  if (screensaverActive) _gifCloseIfOpen();  // force re-open next activation → clean first frame
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
// Returns x for text (= x0 + jpegWidth + 1), or ICON_DRAW_FAILED on failure.
static int drawJpegIcon(const char* path, int* delayMs, int x0 = 0) {
  *delayMs = 1000;
  if (!fsAvailable) return ICON_DRAW_FAILED;
  File jpgFile = LittleFS.open(path);
  if (!jpgFile) {
    Serial.printf("[JPEG] open FAILED: %s\n", path);
    return ICON_DRAW_FAILED;
  }
  TJpgDec.setCallback(jpgMatrixOutput);
  TJpgDec.setJpgScale(1);
  TJpgDec.drawFsJpg(x0, (MATRIX_HEIGHT - 8) / 2, jpgFile);
  jpgFile.close();
  return x0 + 9; // icon width (8) + 1px gap
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

// Internal: render glyphs only — no clear/show
static void _drawGlyphs(const uint8_t glyphs[][5], int count, CRGB color, int xo, int yo) {
  for (int i = 0; i < count; i++)
    for (int row = 0; row < 5; row++)
      for (int col = 0; col < 3; col++)
        if (glyphs[i][row] & (1 << (2 - col)))
          setLED(xo + i * 4 + col, yo + row, color);
}

static void drawStatusWord(const uint8_t glyphs[][5], int count, CRGB color) {
  FastLED.clear();
  int width = count * 4 - 1;
  _drawGlyphs(glyphs, count, color, (MATRIX_WIDTH - width + 1) / 2, (MATRIX_HEIGHT - 5) / 2);
  FastLED.show();
}

// drawUpdate: called once on OTA start — sets text + full dim track on row 7
void drawUpdate() {
  FastLED.clear();
  _drawGlyphs(FONT_UPDATE, 6, LED_COLOR_TIME,
              (MATRIX_WIDTH - 23 + 1) / 2, (MATRIX_HEIGHT - 5) / 2);
  for (int x = 0; x < MATRIX_WIDTH; x++)
    setLED(x, 7, CRGB(0, 25, 25));  // dim track, row 7 only
  FastLED.show();
}

// drawProgress: redraws the full bar row on every call — corrects any corruption
void drawProgress(int barW) {
  for (int x = 0; x < MATRIX_WIDTH; x++)
    setLED(x, 7, (x < barW) ? CRGB::Cyan : CRGB(0, 25, 25));
  FastLED.show();
}

void drawDone()   { drawStatusWord(FONT_DONE,  4, CRGB::Green); }
void drawError()  { drawStatusWord(FONT_ERROR, 5, CRGB::Red);   }

// Boot screen — "ULANZI" in rainbow colours, letters appear one by one.
void drawBootScreen() {
  static const CRGB colors[6] = {
    CRGB(255,   0,   0),  // U — red
    CRGB(255, 100,   0),  // L — orange
    CRGB(200, 200,   0),  // A — yellow
    CRGB(  0, 200,   0),  // N — green
    CRGB(  0, 160, 255),  // Z — cyan-blue
    CRGB(160,   0, 255),  // I — violet
  };
  const char* word = "ULANZI";
  const int xo = (MATRIX_WIDTH  - 23 + 1) / 2;  // centre 23 px across 32
  const int yo = (MATRIX_HEIGHT -  5)     / 2;   // centre 5-row font in 8 rows
  FastLED.clear();
  for (int i = 0; i < 6; i++) {
    drawChar(xo + i * 4, yo, word[i], colors[i]);
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
  if (!autoRotateEnabled || !sht31Available) return;
  if (screensaverActive || iconPreviewActive) return;

  static unsigned long lastRotate = 0;

#if RECV_POCSAG
  static bool prevPocsag = false;
  if (pocsagMsgActive) { prevPocsag = true; return; }  // pause during message
  if (prevPocsag) {                                     // message just ended
    prevPocsag  = false;
    displayMode = MODE_CLOCK;
    lastRotate  = millis();   // restart rotation timer from now
    return;
  }
#endif

  if (millis() - lastRotate < (unsigned long)autoRotateIntervalSec * 1000) return;
  lastRotate  = millis();
  displayMode = (DisplayMode)((displayMode + 1) % MODE_COUNT);
}

// ============================================================
// Main display loop
// ============================================================

void loopDisplay() {
  if (otaInProgress) return;  // OTA owns the display — don't touch it

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
          // Bitmap fallback: 5×5 bell icon at x=0
          for (int row = 0; row < 5; row++)
            for (int col = 0; col < 5; col++)
              if (ICON_MSG[row] & (1 << (4 - col)))
                setLED(col, yo + row, LED_COLOR_POCSAG);
          textX = 6;
          gifDelay = 500;
        }
        _pocsagStaticGifDelay = max(gifDelay, 50);
        // Center text in the space to the right of the icon
        int textW = pocsagMsgLen * 4 - 1;
        int availW = MATRIX_WIDTH - textX;
        int xo = textX + max(0, (availW - textW) / 2);
        for (int i = 0; i < pocsagMsgLen; i++)
          drawChar(xo + i * 4, yo, pocsagMsg[i], LED_COLOR_POCSAG);
        FastLED.show();
        pocsagStaticLastDraw = millis();
        if (first) Serial.printf("[DISP] POCSAG '%s'\n", pocsagMsg);
      }
      if (millis() >= pocsagStaticUntil) {
        pocsagMsgActive      = false;
        screensaverIdleStart = millis();  // restart idle countdown after message
      }
    } else {
      // Scroll: icon and text scroll together, POCSAG_SCROLL_PASSES passes
      if (millis() - pocsagScrollLast < POCSAG_SCROLL_SPEED_MS) return;
      pocsagScrollLast = millis();
      FastLED.clear();
      int gifDelay = POCSAG_SCROLL_SPEED_MS;
      int textX = drawIcon(iconPocsagFile, &gifDelay, pocsagScrollX);
      if (textX == ICON_DRAW_FAILED) {
        // Bitmap fallback: bell at pocsagScrollX
        for (int row = 0; row < 5; row++)
          for (int col = 0; col < 5; col++)
            if (ICON_MSG[row] & (1 << (4 - col)))
              setLED(pocsagScrollX + col, yo + row, LED_COLOR_POCSAG);
        textX = pocsagScrollX + 6;
      }
      for (int i = 0; i < pocsagMsgLen; i++)
        drawChar(textX + i * 4, yo, pocsagMsg[i], LED_COLOR_POCSAG);
      FastLED.show();
      pocsagScrollX--;
      if (pocsagScrollX < -(POCSAG_ICON_RESERVED_PX + pocsagMsgLen * 4)) {
        if (++pocsagScrollPass >= POCSAG_SCROLL_PASSES) {
          pocsagMsgActive      = false;
          screensaverIdleStart = millis();  // restart idle countdown after message
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
    FastLED.show();
    ipScrollX--;
    if (ipScrollX < -(ipScrollLen * 4)) {
      if (++ipScrollPass >= 2)
        ipScrollActive = false;
      else
        ipScrollX = MATRIX_WIDTH;
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
        if (r < 0) { _gif.close(); _gifIsOpen = false; _gifCurPath[0] = '\0'; screensaverActive = false; }
      } else {
        screensaverActive = false;  // file missing/corrupt — abort
      }
    }
    return;
  }
  if (screensaverEnabled && strlen(screensaverFile) > 0) {
    if (millis() - screensaverIdleStart >= (unsigned long)screensaverTimeoutSec * 1000) {
      screensaverActive = true;
      _gifCloseIfOpen();
      FastLED.clear();   // clear display once before first frame draws
      FastLED.show();
      Serial.println("[SS] Screensaver activated");
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
        snprintf(buf, sizeof(buf), "%d.%02dC", t100 / 100, t100 % 100);
      else
        snprintf(buf, sizeof(buf), "-%d.%02dC", (-t100) / 100, (-t100) % 100);
      color = CRGB(255, 120, 0);

      int len   = strlen(buf);
      int textW = len * 4 - 1;
      int textX = drawIcon(iconTempFile, &gifDelay);
      if (textX == ICON_DRAW_FAILED) {
        FastLED.clear();
        gifDelay = 1000;
        int totalW = 4 + textW;
        int xo = (MATRIX_WIDTH - totalW + 1) / 2;
        for (int row = 0; row < 5; row++)
          for (int col = 0; col < 3; col++)
            if (ICON_THERMO[row] & (1 << (2 - col)))
              setLED(xo + col, yo + row, color);
        textX = xo + 4;
      } else {
        for (int x = textX - 1; x < MATRIX_WIDTH; x++)
          for (int y = 0; y < MATRIX_HEIGHT; y++)
            setLED(x, y, CRGB::Black);
      }
      for (int i = 0; i < len; i++)
        drawChar(textX + i * 4, yo, buf[i], color);

    } else {
      int h10 = constrain((int)roundf(sht31Hum * 10.0f), 0, 1000);
      snprintf(buf, sizeof(buf), "%d.%d%%", h10 / 10, h10 % 10);
      color = CRGB(0, 180, 255);

      int len   = strlen(buf);
      int textW = len * 4 - 1;
      int textX = drawIcon(iconHumFile, &gifDelay);
      if (textX == ICON_DRAW_FAILED) {
        FastLED.clear();
        gifDelay = 1000;
        int totalW = 6 + textW;
        int xo = (MATRIX_WIDTH - totalW + 1) / 2;
        for (int row = 0; row < 8; row++)
          for (int col = 0; col < 5; col++)
            if (ICON_DROP[row] & (1 << (4 - col)))
              setLED(xo + col, row, color);
        textX = xo + 6;
      } else {
        for (int x = textX - 1; x < MATRIX_WIDTH; x++)
          for (int y = 0; y < MATRIX_HEIGHT; y++)
            setLED(x, y, CRGB::Black);
      }
      for (int i = 0; i < len; i++)
        drawChar(textX + i * 4, yo, buf[i], color);
    }

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
    CRGB color = batPct > 60 ? CRGB(0, 200, 50) : batPct > 30 ? CRGB(220, 180, 0) : CRGB(220, 40, 0);
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
      const int yf = (MATRIX_HEIGHT - 5) / 2;
      int totalW = 7 + textW;
      int xo = (MATRIX_WIDTH - totalW + 1) / 2;
      for (int row = 0; row < 5; row++)
        for (int col = 0; col < 6; col++)
          if (ICON_BAT[row] & (1 << (5 - col)))
            setLED(xo + col, yf + row, color);
      int fillCols = (batPct * 3 + 50) / 100;
      for (int row = 1; row <= 3; row++)
        for (int col = 1; col <= fillCols; col++)
          setLED(xo + col, yf + row, color);
      textX = xo + 7;
    } else {
      for (int x = textX - 1; x < MATRIX_WIDTH; x++)
        for (int y = 0; y < MATRIX_HEIGHT; y++)
          setLED(x, y, CRGB::Black);
    }
    for (int i = 0; i < len; i++)
      drawChar(textX + i * 4, yo, buf[i], color);
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
  drawTime(t.tm_hour, t.tm_min, t.tm_sec, LED_COLOR_TIME);
  FastLED.show();
}
