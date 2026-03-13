// ── POCSAG screen + shared page-dot indicator ─────────────────────────────────
#pragma once

// Page dots shown on POCSAG and DMR screens (filled = active, outline = inactive)
static void drawPageDots() {
  int cx = 120, y = 220;
  for (int i = 0; i < SCREEN_COUNT; i++) {
    int x = cx + (int)((i - (SCREEN_COUNT - 1) / 2.0) * 12);
    if (i == currentScreen)
      ttgo->tft->fillCircle(x, y, 4, TFT_WHITE);
    else
      ttgo->tft->drawCircle(x, y, 4, TFT_DARKGREY);
  }
}

// Word-wrap text into lines, drawing from (x, startY) down to maxY.
// Uses whatever font is currently set. Returns y after last line drawn.
static int drawWordWrapped(const char* text, int x, int startY, int maxW, int maxY) {
  int lineH   = ttgo->tft->fontHeight() + 3;
  int lineY   = startY;
  int len     = strlen(text);
  int lineStart = 0;   // index into text where current line begins

  while (lineStart < len && lineY + lineH <= maxY) {
    // Find the longest prefix that fits in maxW
    int lastBreak = lineStart;   // last space we can break at
    int end       = lineStart;
    while (end < len) {
      // Measure substring [lineStart, end+1)
      char tmp[POCSAG_MSG_MAX_LEN + 2];
      int sz = end - lineStart + 1;
      if (sz >= (int)sizeof(tmp)) break;
      memcpy(tmp, text + lineStart, sz);
      tmp[sz] = '\0';
      if (ttgo->tft->textWidth(tmp) > maxW) break;
      if (text[end] == ' ') lastBreak = end;
      end++;
    }

    int drawEnd;
    if (end >= len) {
      // Remaining text fits
      drawEnd = len;
    } else if (lastBreak > lineStart) {
      // Break at last space
      drawEnd = lastBreak;
    } else {
      // No space found — hard-break at character boundary
      drawEnd = (end > lineStart) ? end : lineStart + 1;
    }

    char lineBuf[POCSAG_MSG_MAX_LEN + 2];
    int sz = drawEnd - lineStart;
    memcpy(lineBuf, text + lineStart, sz);
    lineBuf[sz] = '\0';
    ttgo->tft->drawString(lineBuf, x, lineY);
    lineY += lineH;

    // Skip the breaking space
    lineStart = drawEnd;
    if (lineStart < len && text[lineStart] == ' ') lineStart++;
  }
  return lineY;
}

static void drawPocsagScreen() {
  ttgo->tft->fillScreen(TFT_BLACK);

  // ── Header ─────────────────────────────────────────────────────────────────
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(TFT_ORANGE, TFT_BLACK);
  ttgo->tft->setTextDatum(TL_DATUM);
  ttgo->tft->drawString("POCSAG", 8, 6);

  if (lastRic > 0) {
    char ricBuf[16];
    snprintf(ricBuf, sizeof(ricBuf), "RIC %u", lastRic);
    ttgo->tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    ttgo->tft->setTextDatum(TR_DATUM);
    ttgo->tft->drawString(ricBuf, 232, 6);
  }

  ttgo->tft->drawFastHLine(0, 26, 240, TFT_DARKGREY);

  // ── Message ────────────────────────────────────────────────────────────────
  if (lastRic > 0 && lastMsg[0] != '\0') {
    ttgo->tft->setTextFont(3);
    ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
    ttgo->tft->setTextDatum(TL_DATUM);
    drawWordWrapped(lastMsg, 8, 32, 224, 210);
  } else if (lastRic == 0) {
    ttgo->tft->setTextFont(2);
    ttgo->tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    ttgo->tft->setTextDatum(MC_DATUM);
    ttgo->tft->drawString("no data", 120, 110);
  }

  drawPageDots();
}
