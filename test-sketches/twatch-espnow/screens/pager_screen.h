// ── Pager screen — last 5 received POCSAG messages ───────────────────────────
// Reads directly from the pocLog[] ring buffer defined in the main sketch.
// Tap a row to open full message view, tap header to return to settings.
#pragma once

#define PAG_BG       TFT_BLACK
#define PAG_ACCENT   0x435B      // royal blue (matches clock hands)
#define PAG_HDR_H    24          // header bar height
#define PAG_ROWS     5
#define PAG_ROW_H    ((240 - PAG_HDR_H) / PAG_ROWS)   // = 43px

static bool     pagerDetailOpen = false;
static uint32_t pagerDetailRic  = 0;
static char     pagerDetailMsg[POCSAG_MSG_MAX_LEN + 1] = "";
static char     pagerDetailTime[24] = "";

static void pagerResetDetailView() {
  pagerDetailOpen = false;
  pagerDetailRic  = 0;
  pagerDetailMsg[0] = '\0';
  pagerDetailTime[0] = '\0';
}

static bool pagerOpenDetailFromRow(int row) {
  if (row < 0 || row >= pocLogCount) return false;
  int idx = ((pocLogHead - 1 - row) % 5 + 5) % 5;
  const PocLogEntry& e = pocLog[idx];
  pagerDetailRic = e.ric;
  strncpy(pagerDetailMsg, e.msg, POCSAG_MSG_MAX_LEN);
  pagerDetailMsg[POCSAG_MSG_MAX_LEN] = '\0';
  strncpy(pagerDetailTime, e.timeStr, sizeof(pagerDetailTime) - 1);
  pagerDetailTime[sizeof(pagerDetailTime) - 1] = '\0';
  pagerDetailOpen = true;
  return true;
}

static void pagerHandleTouch(int16_t tx, int16_t ty) {
  (void)tx;

  if (pagerDetailOpen) {
    // Detail view: tap anywhere to return to list.
    pagerDetailOpen = false;
    needsRedraw = true;
    return;
  }

  // Header tap returns to settings.
  if (ty < PAG_HDR_H) {
    pagerResetDetailView();
    currentScreen = SCREEN_SETTINGS;
    needsRedraw   = true;
    return;
  }

  // Row tap opens detail if that row contains a message.
  int row = (ty - PAG_HDR_H) / PAG_ROW_H;
  if (row >= 0 && row < PAG_ROWS && row < pocLogCount) {
    if (pagerOpenDetailFromRow(row)) {
      needsRedraw = true;
      return;
    }
  }

  // Tap outside valid rows: keep old quick-back behavior.
  pagerResetDetailView();
  currentScreen = SCREEN_SETTINGS;
  needsRedraw   = true;
}

static void drawPagerScreen() {
  if (pagerDetailOpen) {
    ttgo->tft->fillScreen(PAG_BG);

    // Header
    ttgo->tft->fillRect(0, 0, 240, PAG_HDR_H, PAG_ACCENT);
    ttgo->tft->setTextDatum(ML_DATUM);
    ttgo->tft->setTextFont(2);
    ttgo->tft->setTextColor(TFT_WHITE, PAG_ACCENT);
    ttgo->tft->drawString("MESSAGE", 8, PAG_HDR_H / 2);

    ttgo->tft->setTextDatum(MR_DATUM);
    ttgo->tft->setTextFont(1);
    ttgo->tft->setTextColor(0xAD75, PAG_ACCENT);
    ttgo->tft->drawString("tap to list", 234, PAG_HDR_H / 2);

    char ricBuf[16];
    snprintf(ricBuf, sizeof(ricBuf), "RIC %u", pagerDetailRic);

    ttgo->tft->setTextDatum(TL_DATUM);
    ttgo->tft->setTextFont(2);
    ttgo->tft->setTextColor(TFT_YELLOW, PAG_BG);
    ttgo->tft->drawString(ricBuf, 8, 30);

    ttgo->tft->setTextDatum(TR_DATUM);
    ttgo->tft->setTextFont(1);
    ttgo->tft->setTextColor(TFT_DARKGREY, PAG_BG);
    ttgo->tft->drawString(pagerDetailTime, 234, 31);

    ttgo->tft->drawFastHLine(0, 48, 240, TFT_DARKGREY);

    // Full message in larger readable font.
    ttgo->tft->setTextFont(4);
    ttgo->tft->setTextColor(TFT_WHITE, PAG_BG);
    ttgo->tft->setTextDatum(TL_DATUM);
    drawWordWrapped(pagerDetailMsg, 8, 56, 224, 212);

    drawPageDots();
    return;
  }

  ttgo->tft->fillScreen(PAG_BG);

  // ── Header bar ─────────────────────────────────────────────────────────────
  ttgo->tft->fillRect(0, 0, 240, PAG_HDR_H, PAG_ACCENT);
  ttgo->tft->setTextDatum(ML_DATUM);
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(TFT_WHITE, PAG_ACCENT);
  ttgo->tft->drawString("MESSAGES", 8, PAG_HDR_H / 2);

  // Back hint (right side of header)
  ttgo->tft->setTextDatum(MR_DATUM);
  ttgo->tft->setTextFont(1);
  ttgo->tft->setTextColor(0xAD75, PAG_ACCENT);   // light grey
  ttgo->tft->drawString("tap row", 234, PAG_HDR_H / 2);

  // ── Message rows ───────────────────────────────────────────────────────────
  for (int i = 0; i < PAG_ROWS; i++) {
    int yTop = PAG_HDR_H + i * PAG_ROW_H;
    int yMid = yTop + PAG_ROW_H / 2;

    // Separator line above each row (except first)
    if (i > 0)
      ttgo->tft->drawFastHLine(0, yTop, 240, 0x3186);   // dark grey

    if (i >= pocLogCount) {
      // Empty slot
      ttgo->tft->setTextDatum(ML_DATUM);
      ttgo->tft->setTextFont(1);
      ttgo->tft->setTextColor(TFT_DARKGREY, PAG_BG);
      ttgo->tft->drawString("---", 8, yMid);
      continue;
    }

    // Newest entry first: index = (pocLogHead - 1 - i + N*5) % 5
    int idx = ((pocLogHead - 1 - i) % 5 + 5) % 5;
    const PocLogEntry& e = pocLog[idx];

    // Row 1: RIC (left) + time (right)
    int yLine1 = yTop + 10;
    ttgo->tft->setTextFont(2);
    ttgo->tft->setTextColor(TFT_WHITE, PAG_BG);
    ttgo->tft->setTextDatum(ML_DATUM);
    char ricBuf[12];
    snprintf(ricBuf, sizeof(ricBuf), "RIC %u", e.ric);
    ttgo->tft->drawString(ricBuf, 8, yLine1);

    ttgo->tft->setTextFont(1);
    ttgo->tft->setTextColor(TFT_DARKGREY, PAG_BG);
    ttgo->tft->setTextDatum(MR_DATUM);
    ttgo->tft->drawString(e.timeStr, 234, yLine1);

    // Row 2: message text (truncated to fit)
    int yLine2 = yTop + 28;
    ttgo->tft->setTextFont(1);
    ttgo->tft->setTextColor(0xC618, PAG_BG);   // light grey
    ttgo->tft->setTextDatum(ML_DATUM);

    // Truncate message to ~34 chars so it fits in 232px at font 1 (~7px/char)
    char truncBuf[36];
    strncpy(truncBuf, e.msg, 34);
    truncBuf[34] = '\0';
    if (strlen(e.msg) > 34) {
      truncBuf[32] = '.'; truncBuf[33] = '.'; truncBuf[34] = '\0';
    }
    ttgo->tft->drawString(truncBuf, 8, yLine2);
  }
}
