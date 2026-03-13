// ── OTA progress screen ───────────────────────────────────────────────────────
// Wake display, double-buzz, and show a live progress bar during ArduinoOTA.
#pragma once

#define OTA_BAR_X   20
#define OTA_BAR_Y  110
#define OTA_BAR_W  200
#define OTA_BAR_H   20

// Called from onStart — wakes screen, vibrates twice, draws base layout.
static void otaStart() {
  // Wake display
  if (!displayOn) {
    displayOn = true;
    if (ttgo) ttgo->openBL();
  }

  // Double buzz so the user feels the update starting
  ttgo->motor->onec(120);
  delay(180);
  ttgo->motor->onec(120);

  // Base screen
  ttgo->tft->fillScreen(TFT_BLACK);

  ttgo->tft->setTextDatum(MC_DATUM);
  ttgo->tft->setTextFont(4);
  ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
  ttgo->tft->drawString("OTA UPDATE", 120, 60);

  // Empty progress bar outline
  ttgo->tft->drawRect(OTA_BAR_X, OTA_BAR_Y, OTA_BAR_W, OTA_BAR_H, TFT_WHITE);

  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
  ttgo->tft->drawString("Starting...", 120, 148);
}

// Called from onProgress — fills the bar and updates percentage text.
static void otaProgress(unsigned int done, unsigned int total) {
  if (total == 0) return;
  int pct = (int)((done * 100UL) / total);

  // Fill bar (royal blue, same as clock hands)
  int fill = (OTA_BAR_W - 2) * pct / 100;
  ttgo->tft->fillRect(OTA_BAR_X + 1, OTA_BAR_Y + 1, fill,           OTA_BAR_H - 2, 0x435B);
  ttgo->tft->fillRect(OTA_BAR_X + 1 + fill, OTA_BAR_Y + 1,
                      OTA_BAR_W - 2 - fill, OTA_BAR_H - 2, TFT_BLACK);

  // Percentage text
  char buf[8];
  snprintf(buf, sizeof(buf), "%d%%", pct);
  ttgo->tft->setTextDatum(MC_DATUM);
  ttgo->tft->setTextFont(4);
  ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
  ttgo->tft->drawString(buf, 120, 148);
}

// Called from onEnd — show success (device will reboot immediately after).
static void otaDone() {
  ttgo->tft->fillScreen(TFT_BLACK);
  ttgo->tft->setTextDatum(MC_DATUM);
  ttgo->tft->setTextFont(4);
  ttgo->tft->setTextColor(TFT_GREEN, TFT_BLACK);
  ttgo->tft->drawString("Update Done!", 120, 100);
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
  ttgo->tft->drawString("Rebooting...", 120, 140);
}

// Called from onError.
static void otaError(ota_error_t e) {
  ttgo->tft->fillScreen(TFT_BLACK);
  ttgo->tft->setTextDatum(MC_DATUM);
  ttgo->tft->setTextFont(4);
  ttgo->tft->setTextColor(TFT_RED, TFT_BLACK);
  ttgo->tft->drawString("OTA FAILED", 120, 100);
  char buf[20];
  snprintf(buf, sizeof(buf), "Error: %u", (unsigned)e);
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
  ttgo->tft->drawString(buf, 120, 140);
}
