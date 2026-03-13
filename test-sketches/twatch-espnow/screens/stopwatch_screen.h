// screens/stopwatch_screen.h
// Stopwatch — start/stop, lap (up to 4), reset.

// ── State ─────────────────────────────────────────────────────────────────────
static bool          swRunning   = false;
static unsigned long swStartMs   = 0;    // millis() at last start/resume
static unsigned long swElapsedMs = 0;    // accumulated ms before current run
static unsigned long swLapMs[4]  = {};   // lap split times in ms
static int           swLapCount  = 0;

static unsigned long swCurrentMs() {
  return swElapsedMs + (swRunning ? millis() - swStartMs : 0);
}

static void swFormatTime(unsigned long ms, char *buf, size_t len) {
  unsigned long min = ms / 60000;
  unsigned long sec = (ms % 60000) / 1000;
  unsigned long hun = (ms % 1000) / 10;
  snprintf(buf, len, "%02lu:%02lu.%02lu", min, sec, hun);
}

// ── Partial update: time digits only (no flicker) ────────────────────────────
static void updateStopwatchTime() {
  TFT_eSPI *tft = ttgo->tft;
  // Clear only the time region (below title bar, above buttons)
  tft->fillRect(0, 37, 240, 56, TFT_BLACK);
  char timeBuf[16];
  swFormatTime(swCurrentMs(), timeBuf, sizeof(timeBuf));
  tft->setTextFont(6);
  tft->setTextColor(swRunning ? TFT_GREEN : TFT_WHITE, TFT_BLACK);
  tft->setTextDatum(MC_DATUM);
  tft->drawString(timeBuf, 120, 68);
}

// ── Full screen draw (navigation + button state changes) ─────────────────────
static void drawStopwatchScreen() {
  TFT_eSPI *tft = ttgo->tft;
  tft->fillScreen(TFT_BLACK);

  // Title bar
  tft->fillRect(0, 0, 240, 36, 0x1082);
  tft->setTextDatum(MC_DATUM);
  tft->setTextFont(4);
  tft->setTextColor(TFT_WHITE, 0x1082);
  tft->drawString("Stopwatch", 120, 18);

  // Main time display (via shared helper)
  updateStopwatchTime();

  // ── Buttons row ─────────────────────────────────────────────────────────────
  // START/STOP  x=5, y=100, w=110, h=36
  uint16_t ssbg = swRunning ? 0x7800 : 0x03C0;
  tft->fillRoundRect(5, 100, 110, 36, 8, ssbg);
  tft->drawRoundRect(5, 100, 110, 36, 8, TFT_WHITE);
  tft->setTextFont(2);
  tft->setTextColor(TFT_WHITE, ssbg);
  tft->setTextDatum(MC_DATUM);
  tft->drawString(swRunning ? "STOP" : "START", 60, 118);

  // LAP  x=125, y=100, w=110, h=36
  uint16_t lapbg = (swRunning && swLapCount < 4) ? TFT_NAVY : 0x2104;
  tft->fillRoundRect(125, 100, 110, 36, 8, lapbg);
  tft->drawRoundRect(125, 100, 110, 36, 8, TFT_WHITE);
  tft->setTextColor(TFT_WHITE, lapbg);
  tft->drawString("LAP", 180, 118);

  // RESET  x=5, y=142, w=110, h=28
  tft->fillRoundRect(5, 142, 110, 28, 8, 0x4208);
  tft->drawRoundRect(5, 142, 110, 28, 8, TFT_WHITE);
  tft->setTextColor(TFT_WHITE, 0x4208);
  tft->drawString("RESET", 60, 156);

  // BACK  x=125, y=142, w=110, h=28
  tft->fillRoundRect(125, 142, 110, 28, 8, TFT_NAVY);
  tft->drawRoundRect(125, 142, 110, 28, 8, TFT_WHITE);
  tft->setTextColor(TFT_WHITE, TFT_NAVY);
  tft->drawString("BACK", 180, 156);

  // ── Lap list ────────────────────────────────────────────────────────────────
  tft->setTextFont(1);
  tft->setTextDatum(TL_DATUM);
  for (int i = 0; i < swLapCount; i++) {
    char lapBuf[24];
    char lapTime[12];
    swFormatTime(swLapMs[i], lapTime, sizeof(lapTime));
    snprintf(lapBuf, sizeof(lapBuf), "Lap %d: %s", i + 1, lapTime);
    uint16_t lcol = (i == swLapCount - 1) ? TFT_YELLOW : 0x8410;
    tft->setTextColor(lcol, TFT_BLACK);
    tft->drawString(lapBuf, 10, 180 + i * 14);
  }
}

// ── Touch handler ─────────────────────────────────────────────────────────────
static void stopwatchHandleTouch(int16_t tx, int16_t ty) {
  // START/STOP  x=5..115, y=100..136
  if (tx >= 5 && tx <= 115 && ty >= 100 && ty <= 136) {
    if (swRunning) {
      swElapsedMs = swCurrentMs();
      swRunning   = false;
    } else {
      swStartMs = millis();
      swRunning = true;
    }
    needsRedraw = true;
    return;
  }

  // LAP  x=125..235, y=100..136
  if (tx >= 125 && tx <= 235 && ty >= 100 && ty <= 136) {
    if (swRunning && swLapCount < 4) {
      swLapMs[swLapCount++] = swCurrentMs();
      needsRedraw = true;
    }
    return;
  }

  // RESET  x=5..115, y=142..170
  if (tx >= 5 && tx <= 115 && ty >= 142 && ty <= 170) {
    swRunning   = false;
    swElapsedMs = 0;
    swStartMs   = 0;
    swLapCount  = 0;
    memset(swLapMs, 0, sizeof(swLapMs));
    needsRedraw = true;
    return;
  }

  // BACK  x=125..235, y=142..170
  if (tx >= 125 && tx <= 235 && ty >= 142 && ty <= 170) {
    currentScreen = SCREEN_SETTINGS;
    needsRedraw   = true;
  }
}
