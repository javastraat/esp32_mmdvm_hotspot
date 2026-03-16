// display.ino — Font tables, LED drawing helpers, brightness, auto-rotate,
// and the main display loop.
// All globals (leds[], displayMode, pocsagMsg*, currentBrightness, sht31Temp/Hum,
// otaInProgress, timeSynced, autoRotateEnabled, etc.) declared in ulanzi-espnow.ino.

// ============================================================
// 3×5 pixel fonts
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

// ============================================================
// LED drawing primitives
// ============================================================

static void setLED(int x, int y, CRGB color) {
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
static void drawUpdate() {
  FastLED.clear();
  _drawGlyphs(FONT_UPDATE, 6, LED_COLOR_TIME,
              (MATRIX_WIDTH - 23 + 1) / 2, (MATRIX_HEIGHT - 5) / 2);
  for (int x = 0; x < MATRIX_WIDTH; x++)
    setLED(x, 7, CRGB(0, 25, 25));  // dim track, row 7 only
  FastLED.show();
}

// drawProgress: redraws the full bar row on every call — corrects any corruption
static void drawProgress(int barW) {
  for (int x = 0; x < MATRIX_WIDTH; x++)
    setLED(x, 7, (x < barW) ? CRGB::Cyan : CRGB(0, 25, 25));
  FastLED.show();
}

static void drawDone()   { drawStatusWord(FONT_DONE,  4, CRGB::Green); }
static void drawError()  { drawStatusWord(FONT_ERROR, 5, CRGB::Red);   }

// Boot screen — "ULANZI" in rainbow colours, letters appear one by one.
// Called from setup(); setup() clears the frame immediately after return so
// the scanner animation takes over as soon as loop() starts.
static void drawBootScreen() {
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

static void drawChar(int x, int y, char c, CRGB color) {
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

static void loopBrightness() {
  if (!autoBrightnessEnabled) return;
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < LDR_UPDATE_MS) return;
  lastUpdate = millis();

  int ldr = constrain(analogRead(LDR_PIN), LDR_ADC_DARK, LDR_ADC_BRIGHT);
  uint8_t target = (uint8_t)map(ldr, LDR_ADC_DARK, LDR_ADC_BRIGHT, LDR_MIN_BRIGHTNESS, 255);
  // EMA smoothing: blend 1/4 toward target each sample
  currentBrightness = (uint8_t)((currentBrightness * 3 + target + 2) / 4);
  FastLED.setBrightness(currentBrightness);
}

// ============================================================
// Auto-rotation — cycles clock→temp→humidity on a timer
// ============================================================

static void loopAutoRotate() {
  if (!autoRotateEnabled || !sht31Available) return;

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

static void loopDisplay() {
  if (otaInProgress) return;  // OTA owns the display — don't touch it

  // POCSAG message display — takes priority over all modes
#if RECV_POCSAG
  if (pocsagMsgActive) {
    const int yo = (MATRIX_HEIGHT - 5) / 2;
    if (!pocsagIsScrolling) {
      // Static: redraw every 500 ms so a clock tick can't erase it
      if (millis() - pocsagStaticLastDraw >= 500) {
        bool first = (pocsagStaticLastDraw == 0);
        pocsagStaticLastDraw = millis();
        int totalW = pocsagMsgLen * 4 - 1;
        int xo = (MATRIX_WIDTH - totalW) / 2;
        FastLED.clear();
        for (int i = 0; i < pocsagMsgLen; i++)
          drawChar(xo + i * 4, yo, pocsagMsg[i], LED_COLOR_POCSAG);
        FastLED.show();
        if (first)
          Serial.printf("[DISP] POCSAG '%s'\n", pocsagMsg);
      }
      if (millis() >= pocsagStaticUntil)
        pocsagMsgActive = false;
    } else {
      // Scroll 3 passes, 50 ms per pixel
      if (millis() - pocsagScrollLast < POCSAG_SCROLL_SPEED_MS) return;
      pocsagScrollLast = millis();
      FastLED.clear();
      for (int i = 0; i < pocsagMsgLen; i++)
        drawChar(pocsagScrollX + i * 4, yo, pocsagMsg[i], LED_COLOR_POCSAG);
      FastLED.show();
      pocsagScrollX--;
      if (pocsagScrollX < -(pocsagMsgLen * 4)) {
        if (++pocsagScrollPass >= POCSAG_SCROLL_PASSES)
          pocsagMsgActive = false;
        else
          pocsagScrollX = MATRIX_WIDTH;
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

  // Temperature / humidity display
  if (displayMode == MODE_TEMP || displayMode == MODE_HUMIDITY) {
    static DisplayMode   lastMode = MODE_CLOCK;
    static unsigned long lastDraw = 0;
    bool modeChanged = (lastMode != displayMode);
    lastMode = displayMode;

    if (!modeChanged && millis() - lastDraw < 1000) return;
    lastDraw = millis();

    const int yo = (MATRIX_HEIGHT - 5) / 2;  // vertically centre the text (=1)
    char buf[8];
    CRGB color;
    FastLED.clear();

    if (displayMode == MODE_TEMP) {
      int t100 = (int)roundf(sht31Temp * 100.0f);
      if (t100 >= 0)
        snprintf(buf, sizeof(buf), "%d.%02dC", t100 / 100, t100 % 100);
      else
        snprintf(buf, sizeof(buf), "-%d.%02dC", (-t100) / 100, (-t100) % 100);
      color = CRGB(255, 120, 0);   // warm orange

      // 3×5 thermometer icon + text, centred together
      int len    = strlen(buf);
      int totalW = 4 + len * 4 - 1;  // 3px icon + 1px gap + text
      int xo     = (MATRIX_WIDTH - totalW + 1) / 2;
      for (int row = 0; row < 5; row++)
        for (int col = 0; col < 3; col++)
          if (ICON_THERMO[row] & (1 << (2 - col)))
            setLED(xo + col, yo + row, color);
      for (int i = 0; i < len; i++)
        drawChar(xo + 4 + i * 4, yo, buf[i], color);

    } else {
      // 1 decimal place keeps max width to 6 chars ("100.0%") so icon fits
      int h10 = constrain((int)roundf(sht31Hum * 10.0f), 0, 1000);
      snprintf(buf, sizeof(buf), "%d.%d%%", h10 / 10, h10 % 10);
      color = CRGB(0, 180, 255);   // cyan-blue

      // 5×8 water drop icon (full matrix height) + text centred vertically
      int len    = strlen(buf);
      int totalW = 6 + len * 4 - 1;  // 5px icon + 1px gap + text
      int xo     = (MATRIX_WIDTH - totalW + 1) / 2;
      for (int row = 0; row < 8; row++)
        for (int col = 0; col < 5; col++)
          if (ICON_DROP[row] & (1 << (4 - col)))
            setLED(xo + col, row, color);
      for (int i = 0; i < len; i++)
        drawChar(xo + 6 + i * 4, yo, buf[i], color);
    }

    FastLED.show();
    return;
  }

  // Battery display
  if (displayMode == MODE_BATTERY) {
    static unsigned long lastDraw = 0;
    if (millis() - lastDraw < 2000) return;
    lastDraw = millis();

    int batRaw = analogRead(BAT_PIN);
    int batPct = (int)constrain(map(batRaw, BAT_RAW_EMPTY, BAT_RAW_FULL, 0, 100), 0, 100);
    CRGB color = batPct > 60 ? CRGB(0, 200, 50) : batPct > 30 ? CRGB(220, 180, 0) : CRGB(220, 40, 0);

    char buf[5];
    snprintf(buf, sizeof(buf), "%d%%", batPct);
    int len    = strlen(buf);
    int totalW = 7 + len * 4 - 1;  // 6px icon + 1px gap + text
    int xo     = (MATRIX_WIDTH - totalW + 1) / 2;
    const int yo = (MATRIX_HEIGHT - 5) / 2;

    FastLED.clear();
    // Battery outline (6 wide, 5 tall, centred vertically)
    for (int row = 0; row < 5; row++)
      for (int col = 0; col < 6; col++)
        if (ICON_BAT[row] & (1 << (5 - col)))
          setLED(xo + col, yo + row, color);
    // Fill interior (cols 1–3, rows 1–3) based on level
    int fillCols = (batPct * 3 + 50) / 100;  // 0..3
    for (int row = 1; row <= 3; row++)
      for (int col = 1; col <= fillCols; col++)
        setLED(xo + col, yo + row, color);
    // Text
    for (int i = 0; i < len; i++)
      drawChar(xo + 7 + i * 4, yo, buf[i], color);
    FastLED.show();
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
