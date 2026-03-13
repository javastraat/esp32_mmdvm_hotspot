// ── Pager screen — last 5 received POCSAG messages ───────────────────────────
// Reads directly from the pocLog[] ring buffer defined in the main sketch.
// Tap anywhere to return to the settings screen.
#pragma once

#define PAG_BG       TFT_BLACK
#define PAG_ACCENT   0x435B      // royal blue (matches clock hands)
#define PAG_HDR_H    24          // header bar height
#define PAG_ROWS     5
#define PAG_ROW_H    ((240 - PAG_HDR_H) / PAG_ROWS)   // = 43px

static void drawPagerScreen() {
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
  ttgo->tft->drawString("tap to back", 234, PAG_HDR_H / 2);

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
