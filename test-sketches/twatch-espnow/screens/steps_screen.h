// screens/steps_screen.h
// Step counter screen — reads BMA423 pedometer, shows progress toward daily goal.

#define STEPS_DAILY_GOAL  10000

static void drawStepsScreen() {
  TFT_eSPI *tft = ttgo->tft;
  tft->fillScreen(TFT_BLACK);

  // Title bar
  tft->fillRect(0, 0, 240, 36, 0x1082);
  tft->setTextDatum(MC_DATUM);
  tft->setTextFont(4);
  tft->setTextColor(TFT_WHITE, 0x1082);
  tft->drawString("Steps", 120, 18);

  // Step count
  uint32_t steps = ttgo->bma->getCounter();
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)steps);

  tft->setTextDatum(MC_DATUM);
  tft->setTextFont(7);   // large 7-segment style
  tft->setTextColor(TFT_CYAN, TFT_BLACK);
  tft->drawString(buf, 120, 95);

  // Goal label
  tft->setTextFont(2);
  tft->setTextColor(0x8410, TFT_BLACK);
  tft->setTextDatum(MC_DATUM);
  tft->drawString("Goal: 10,000 steps", 120, 135);

  // Progress bar  x=10, y=148, w=220, h=14
  tft->drawRoundRect(10, 148, 220, 14, 4, 0x4208);
  int prog = (int)(min((uint32_t)STEPS_DAILY_GOAL, steps) * 218UL / STEPS_DAILY_GOAL);
  if (prog > 0) {
    uint16_t barColor = steps >= STEPS_DAILY_GOAL ? TFT_GREEN : TFT_CYAN;
    tft->fillRoundRect(11, 149, prog, 12, 3, barColor);
  }

  // Percentage
  int pct = (int)(min((uint32_t)STEPS_DAILY_GOAL, steps) * 100UL / STEPS_DAILY_GOAL);
  char pctBuf[8];
  snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
  tft->setTextFont(2);
  tft->setTextColor(TFT_WHITE, TFT_BLACK);
  tft->setTextDatum(MC_DATUM);
  tft->drawString(pctBuf, 120, 172);

  // RESET button  x=10, y=186, w=100, h=28
  tft->fillRoundRect(10, 186, 100, 28, 8, 0x7800);
  tft->drawRoundRect(10, 186, 100, 28, 8, TFT_WHITE);
  tft->setTextFont(2);
  tft->setTextColor(TFT_WHITE, 0x7800);
  tft->setTextDatum(MC_DATUM);
  tft->drawString("RESET", 60, 200);

  // BACK button  x=130, y=186, w=100, h=28
  tft->fillRoundRect(130, 186, 100, 28, 8, TFT_NAVY);
  tft->drawRoundRect(130, 186, 100, 28, 8, TFT_WHITE);
  tft->setTextColor(TFT_WHITE, TFT_NAVY);
  tft->drawString("BACK", 180, 200);
}

static void stepsHandleTouch(int16_t tx, int16_t ty) {
  // RESET  x=10..110, y=186..214
  if (tx >= 10 && tx <= 110 && ty >= 186 && ty <= 214) {
    ttgo->bma->resetStepCounter();
    needsRedraw = true;
    return;
  }
  // BACK  x=130..230, y=186..214
  if (tx >= 130 && tx <= 230 && ty >= 186 && ty <= 214) {
    currentScreen = SCREEN_SETTINGS;
    needsRedraw   = true;
  }
}
