// ── Watchface + time-format editor screen ────────────────────────────────────
// Choose watchface layout and 12/24h display mode, then save both together.
#pragma once

#define WF_BG       TFT_BLACK
#define WF_ACCENT   0x435B      // royal blue
#define WF_HDR_H    24

// Card geometry
#define WF_CARD_Y   30
#define WF_CARD_H  116
#define WF_CARD_W  109
#define WF_CARD_L1   5          // left card x start
#define WF_CARD_L2 126          // right card x start

// 12/24h selector
#define WF_FMT_Y    160
#define WF_FMT_H     30
#define WF_FMT_W    100
#define WF_FMT_LX    10
#define WF_FMT_RX   130

// Bottom action buttons
#define WF_BTN_Y    204
#define WF_BTN_H     30
#define WF_BTN_W    100
#define WF_BTN_BX    10
#define WF_BTN_SX   130

static uint8_t wfDraftWatchface = DEFAULT_WATCHFACE;
static bool    wfDraft24h       = DEFAULT_CLOCK_24H;
static bool    wfEditorOpen     = false;

static void watchfaceEditBegin() {
  wfDraftWatchface = watchfaceId;
  wfDraft24h       = clock24h;
  wfEditorOpen     = true;
}

static void watchfaceEditEnd() {
  wfEditorOpen = false;
}

static void wfEnsureEditorState() {
  if (!wfEditorOpen) watchfaceEditBegin();
}

static bool wfHit(int16_t tx, int16_t ty, int x, int y, int w, int h) {
  return (tx >= x && tx <= x + w && ty >= y && ty <= y + h);
}

// Draw a mini analog clock preview centred at (cx, cy) with radius R.
static void wf_miniAnalog(int cx, int cy, int R, uint32_t color) {
  ttgo->tft->drawCircle(cx, cy, R, color);

  // Four major tick marks (12, 3, 6, 9)
  for (int i = 0; i < 4; i++) {
    float a = (i * 90.0f - 90.0f) * DEG_TO_RAD;
    ttgo->tft->drawLine(
      cx + (int)((R - 4) * cosf(a)), cy + (int)((R - 4) * sinf(a)),
      cx + (int)( R      * cosf(a)), cy + (int)( R      * sinf(a)),
      color);
  }

  // Hands at 10:10 — classic symmetrical display position
  // Minute hand at 10 min → 60° clockwise from 12 → standard rad = -30°
  float mRad = (60.0f - 90.0f) * DEG_TO_RAD;
  ttgo->tft->drawLine(cx, cy,
    cx + (int)(R * 0.72f * cosf(mRad)), cy + (int)(R * 0.72f * sinf(mRad)), color);

  // Hour hand at 10h 10m → 305° clockwise from 12 → standard rad = 215°
  float hRad = (305.0f - 90.0f) * DEG_TO_RAD;
  ttgo->tft->drawLine(cx, cy,
    cx + (int)(R * 0.52f * cosf(hRad)), cy + (int)(R * 0.52f * sinf(hRad)), color);

  ttgo->tft->fillCircle(cx, cy, 2, color);
}

static void watchfaceHandleTouch(int16_t tx, int16_t ty) {
  wfEnsureEditorState();

  if (wfHit(tx, ty, WF_CARD_L1, WF_CARD_Y, WF_CARD_W, WF_CARD_H)) {
    if (wfDraftWatchface != 0) {
      wfDraftWatchface = 0;
      needsRedraw = true;
    }
    return;
  }

  if (wfHit(tx, ty, WF_CARD_L2, WF_CARD_Y, WF_CARD_W, WF_CARD_H)) {
    if (wfDraftWatchface != 1) {
      wfDraftWatchface = 1;
      needsRedraw = true;
    }
    return;
  }

  if (wfHit(tx, ty, WF_FMT_LX, WF_FMT_Y, WF_FMT_W, WF_FMT_H)) {
    if (wfDraft24h) {
      wfDraft24h = false;
      needsRedraw = true;
    }
    return;
  }

  if (wfHit(tx, ty, WF_FMT_RX, WF_FMT_Y, WF_FMT_W, WF_FMT_H)) {
    if (!wfDraft24h) {
      wfDraft24h = true;
      needsRedraw = true;
    }
    return;
  }

  if (wfHit(tx, ty, WF_BTN_BX, WF_BTN_Y, WF_BTN_W, WF_BTN_H)) {
    watchfaceEditEnd();
    currentScreen = SCREEN_SETTINGS;
    needsRedraw   = true;
    return;
  }

  if (wfHit(tx, ty, WF_BTN_SX, WF_BTN_Y, WF_BTN_W, WF_BTN_H)) {
    saveWatchfaceSettings(wfDraftWatchface, wfDraft24h);
    watchfaceEditEnd();
    currentScreen = SCREEN_SETTINGS;
    needsRedraw   = true;
    return;
  }
}

static void drawWatchfaceScreen() {
  wfEnsureEditorState();
  ttgo->tft->fillScreen(WF_BG);

  // ── Header ─────────────────────────────────────────────────────────────────
  ttgo->tft->fillRect(0, 0, 240, WF_HDR_H, WF_ACCENT);
  ttgo->tft->setTextDatum(ML_DATUM);
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(TFT_WHITE, WF_ACCENT);
  ttgo->tft->drawString("WATCHFACE", 8, WF_HDR_H / 2);
  ttgo->tft->setTextDatum(MR_DATUM);
  ttgo->tft->drawString("layout + time", 232, WF_HDR_H / 2);

  // ── Draw each card ─────────────────────────────────────────────────────────
  const int cards[2] = { WF_CARD_L1, WF_CARD_L2 };
  const char* labels[2] = { "Analog", "Digital" };

  for (int i = 0; i < 2; i++) {
    int x  = cards[i];
    int cx = x + WF_CARD_W / 2;
    int cy = WF_CARD_Y + WF_CARD_H / 2 - 8;

    bool selected = ((uint8_t)i == wfDraftWatchface);
    uint32_t borderCol = selected ? WF_ACCENT : 0x2104;  // blue or dark grey

    // Card border (2px)
    ttgo->tft->drawRect(x,   WF_CARD_Y,   WF_CARD_W,   WF_CARD_H,   borderCol);
    ttgo->tft->drawRect(x+1, WF_CARD_Y+1, WF_CARD_W-2, WF_CARD_H-2, borderCol);

    // Preview content
    uint32_t fgCol = selected ? WF_ACCENT : TFT_WHITE;

    if (i == 0) {
      // Analog: mini clock drawing
      wf_miniAnalog(cx, cy - 2, 30, fgCol);
    } else {
      // Digital preview reflects selected 12/24h mode.
      const char* mainTime = wfDraft24h ? "23:45" : "11:45";
      ttgo->tft->setTextDatum(MC_DATUM);
      ttgo->tft->setTextFont(4);
      ttgo->tft->setTextColor(fgCol, WF_BG);
      ttgo->tft->drawString(mainTime, cx, cy - 12);
      ttgo->tft->setTextFont(2);
      ttgo->tft->setTextColor(selected ? WF_ACCENT : TFT_DARKGREY, WF_BG);
      ttgo->tft->drawString(wfDraft24h ? "24H" : "PM", cx, cy + 14);
    }

    // Label below preview
    ttgo->tft->setTextDatum(MC_DATUM);
    ttgo->tft->setTextFont(2);
    ttgo->tft->setTextColor(fgCol, WF_BG);
    ttgo->tft->drawString(labels[i], cx, WF_CARD_Y + WF_CARD_H - 14);

    // Selected indicator dot at top of card
    if (selected) {
      ttgo->tft->fillCircle(cx, WF_CARD_Y + 9, 4, WF_ACCENT);
    }
  }

  // ── 12h / 24h selector ─────────────────────────────────────────────────────
  ttgo->tft->setTextDatum(MC_DATUM);
  ttgo->tft->setTextFont(2);
  // ttgo->tft->setTextColor(TFT_DARKGREY, WF_BG);
  // ttgo->tft->drawString("TIME FORMAT", 120, 146);

  uint32_t c12 = wfDraft24h ? 0x2104 : WF_ACCENT;
  uint32_t c24 = wfDraft24h ? WF_ACCENT : 0x2104;

  ttgo->tft->fillRoundRect(WF_FMT_LX, WF_FMT_Y, WF_FMT_W, WF_FMT_H, 10, c12);
  ttgo->tft->fillRoundRect(WF_FMT_RX, WF_FMT_Y, WF_FMT_W, WF_FMT_H, 10, c24);

  ttgo->tft->setTextColor(TFT_WHITE, c12);
  ttgo->tft->drawString("12H", WF_FMT_LX + WF_FMT_W / 2, WF_FMT_Y + WF_FMT_H / 2);
  ttgo->tft->setTextColor(TFT_WHITE, c24);
  ttgo->tft->drawString("24H", WF_FMT_RX + WF_FMT_W / 2, WF_FMT_Y + WF_FMT_H / 2);

  // ── Action buttons ─────────────────────────────────────────────────────────
  ttgo->tft->fillRoundRect(WF_BTN_BX, WF_BTN_Y, WF_BTN_W, WF_BTN_H, 10, TFT_DARKGREY);
  ttgo->tft->fillRoundRect(WF_BTN_SX, WF_BTN_Y, WF_BTN_W, WF_BTN_H, 10, TFT_GREEN);

  ttgo->tft->setTextColor(TFT_WHITE, TFT_DARKGREY);
  ttgo->tft->drawString("Back", WF_BTN_BX + WF_BTN_W / 2, WF_BTN_Y + WF_BTN_H / 2);
  ttgo->tft->setTextColor(TFT_BLACK, TFT_GREEN);
  ttgo->tft->drawString("Save", WF_BTN_SX + WF_BTN_W / 2, WF_BTN_Y + WF_BTN_H / 2);
}
