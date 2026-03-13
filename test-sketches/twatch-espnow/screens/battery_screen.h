// ── Battery info screen ───────────────────────────────────────────────────────
// Pages:  0 = Basic (Connected, Charge, Voltage, Status)
//         1 = Power  (Current, Power, Coulomb, PMIC Temp)
//         2 = USB    (USB Voltage, USB Current, USB Power) — only when USB present
// Tap body → next page (cycles).   Tap header → back to settings.
#pragma once

#define BAT_BG      TFT_BLACK
#define BAT_ACCENT  0x435B      // royal blue
#define BAT_HDR_H   28

static uint8_t batPage = 0;

// Row layout constants shared by draw and update
static const int BAT_Y0 = 62;
static const int BAT_DY = 46;

// Labelled row — font 2 label (left), font 4 value (right)
static void batRow(const char* label, const char* value, int ly, uint16_t valColor = TFT_WHITE) {
  ttgo->tft->fillRect(0, ly - 14, 240, 28, BAT_BG);
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextDatum(ML_DATUM);
  ttgo->tft->setTextColor(TFT_DARKGREY, BAT_BG);
  ttgo->tft->drawString(label, 10, ly);
  ttgo->tft->setTextFont(4);
  ttgo->tft->setTextDatum(MR_DATUM);
  ttgo->tft->setTextColor(valColor, BAT_BG);
  ttgo->tft->drawString(value, 232, ly);
}

// Section divider
static void batSection(const char* title, int y) {
  ttgo->tft->drawFastHLine(0, y - 2, 240, 0x2104);
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextDatum(ML_DATUM);
  ttgo->tft->setTextColor(BAT_ACCENT, BAT_BG);
  ttgo->tft->drawString(title, 10, y + 8);
}

// ── Data-only update (no fillScreen, no header, no dots) ─────────────────────
static void updateBatteryData() {
  TFT_eSPI *tft = ttgo->tft;

  bool  batConn  = ttgo->power->isBatteryConnect();
  int   pct      = batConn ? (int)ttgo->power->getBattPercentage() : 0;
  float batVolt  = batConn ? ttgo->power->getBattVoltage()         : 0.0f;
  bool  charging = ttgo->power->isChargeing();
  float batCur   = charging
                     ?  ttgo->power->getBattChargeCurrent()
                     : -ttgo->power->getBattDischargeCurrent();
  float powerMw  = (batVolt * fabsf(batCur)) / 1000.0f;
  float coulomb  = ttgo->power->getCoulombData();
  float pmuTemp  = ttgo->power->getTemp();
  bool  usbIn    = ttgo->power->isVBUSPlug();

  uint8_t maxPage = usbIn ? 2 : 1;
  if (batPage > maxPage) batPage = 0;

  if (batPage == 0) {
    batRow("Connected",
           batConn ? "Yes" : "No", BAT_Y0,
           batConn ? TFT_GREEN : TFT_RED);

    {
      char buf[12];
      snprintf(buf, sizeof(buf), "%d %%", pct);
      uint16_t col = pct > 50 ? TFT_GREEN : (pct > 20 ? TFT_YELLOW : TFT_RED);
      batRow("Charge", buf, BAT_Y0 + BAT_DY, col);
      int bx = 10, by = BAT_Y0 + BAT_DY + 16, bw = 220, bh = 8;
      tft->fillRect(bx, by, bw, bh, BAT_BG);           // clear bar area
      tft->drawRect(bx, by, bw, bh, 0x4208);
      int fill = pct * (bw - 2) / 100;
      if (fill > 0) tft->fillRect(bx + 1, by + 1, fill, bh - 2, col);
    }

    {
      char buf[14];
      snprintf(buf, sizeof(buf), "%d mV", (int)batVolt);
      batRow("Voltage", buf, BAT_Y0 + BAT_DY * 2 + 8);
    }

    batRow("Status",
           charging ? "Charging" : "Discharging", BAT_Y0 + BAT_DY * 3 + 8,
           charging ? TFT_GREEN : TFT_WHITE);

  } else if (batPage == 1) {
    {
      char buf[16];
      snprintf(buf, sizeof(buf), "%+d mA", (int)batCur);
      batRow("Current", buf, BAT_Y0, charging ? TFT_GREEN : TFT_CYAN);
    }
    {
      char buf[14];
      snprintf(buf, sizeof(buf), "%.0f mW", powerMw);
      batRow("Power", buf, BAT_Y0 + BAT_DY);
    }
    {
      char buf[16];
      snprintf(buf, sizeof(buf), "%.1f mAh", coulomb);
      uint16_t col = coulomb >= 0 ? TFT_GREEN : TFT_ORANGE;
      batRow("Coulomb", buf, BAT_Y0 + BAT_DY * 2, col);
    }
    {
      char buf[12];
      snprintf(buf, sizeof(buf), "%.1f C", pmuTemp);
      uint16_t col = pmuTemp < 45.0f ? TFT_WHITE : (pmuTemp < 60.0f ? TFT_YELLOW : TFT_RED);
      batRow("PMIC Temp", buf, BAT_Y0 + BAT_DY * 3, col);
    }

  } else {
    float usbVolt = ttgo->power->getVbusVoltage();
    float usbCur  = ttgo->power->getVbusCurrent();
    float usbPw   = (usbVolt * usbCur) / 1000.0f;

    {
      char buf[14];
      snprintf(buf, sizeof(buf), "%d mV", (int)usbVolt);
      batRow("USB Voltage", buf, BAT_Y0);
    }
    {
      char buf[14];
      snprintf(buf, sizeof(buf), "%d mA", (int)usbCur);
      batRow("USB Current", buf, BAT_Y0 + BAT_DY);
    }
    {
      char buf[14];
      snprintf(buf, sizeof(buf), "%.0f mW", usbPw);
      batRow("USB Power", buf, BAT_Y0 + BAT_DY * 2);
    }
  }
}

// ── Full screen draw (called on navigation / page change) ─────────────────────
static void drawBatteryScreen() {
  TFT_eSPI *tft = ttgo->tft;
  tft->fillScreen(BAT_BG);

  bool usbIn    = ttgo->power->isVBUSPlug();
  uint8_t maxPage = usbIn ? 2 : 1;
  if (batPage > maxPage) batPage = 0;

  // Header
  tft->fillRect(0, 0, 240, BAT_HDR_H, BAT_ACCENT);
  tft->setTextFont(2);
  tft->setTextColor(TFT_WHITE, BAT_ACCENT);
  const char* titles[] = { "BATTERY 1/2", "BATTERY 2/2", "USB POWER" };
  tft->setTextDatum(ML_DATUM);
  tft->drawString(titles[batPage], 8, BAT_HDR_H / 2);
  tft->setTextDatum(MR_DATUM);
  tft->drawString("back", 232, BAT_HDR_H / 2);

  // Section title
  const char* sections[] = { "Battery", "Power", "USB" };
  batSection(sections[batPage], 34);

  // Page indicator dots
  int dotX = usbIn ? 114 : 117;
  for (int i = 0; i <= (int)maxPage; i++) {
    int x = dotX + i * 12;
    if (i == batPage) tft->fillCircle(x, 230, 5, TFT_WHITE);
    else              tft->drawCircle(x, 230, 5, TFT_DARKGREY);
  }

  // Data rows
  updateBatteryData();
}

static void batteryHandleTouch(int16_t tx, int16_t ty) {
  if (ty < BAT_HDR_H) {
    batPage       = 0;
    currentScreen = SCREEN_SETTINGS;
    needsRedraw   = true;
  } else {
    bool usbIn      = ttgo->power->isVBUSPlug();
    uint8_t maxPage = usbIn ? 2 : 1;
    batPage         = (batPage + 1) % (maxPage + 1);
    needsRedraw     = true;   // page changed → full redraw needed
  }
}
