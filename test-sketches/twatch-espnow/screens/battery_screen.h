// ── Battery info screen ───────────────────────────────────────────────────────
// Shows all AXP202 power readings. USB section only appears when USB is present.
// Tap anywhere to return to the settings screen.
#pragma once

#define BAT_BG      TFT_BLACK
#define BAT_ACCENT  0x435B      // royal blue (matches clock hands)
#define BAT_HDR_H   24

// Helper: draw a labelled row  "Label   Value"
//   ly = y-centre of the row
static void batRow(const char* label, const char* value, int ly, uint32_t valColor = TFT_WHITE) {
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextDatum(ML_DATUM);
  ttgo->tft->setTextColor(TFT_DARKGREY, BAT_BG);
  ttgo->tft->drawString(label, 10, ly);

  ttgo->tft->setTextDatum(MR_DATUM);
  ttgo->tft->setTextColor(valColor, BAT_BG);
  ttgo->tft->drawString(value, 230, ly);
}

// Section divider line + label
static void batSection(const char* title, int y) {
  ttgo->tft->drawFastHLine(0, y - 2, 240, 0x2104);   // dim grey line
  ttgo->tft->setTextFont(1);
  ttgo->tft->setTextDatum(ML_DATUM);
  ttgo->tft->setTextColor(BAT_ACCENT, BAT_BG);
  ttgo->tft->drawString(title, 10, y + 6);
}

static void drawBatteryScreen() {
  ttgo->tft->fillScreen(BAT_BG);

  // ── Header bar ─────────────────────────────────────────────────────────────
  ttgo->tft->fillRect(0, 0, 240, BAT_HDR_H, BAT_ACCENT);
  ttgo->tft->setTextDatum(ML_DATUM);
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(TFT_WHITE, BAT_ACCENT);
  ttgo->tft->drawString("BATTERY", 8, BAT_HDR_H / 2);
  ttgo->tft->setTextDatum(MR_DATUM);
  ttgo->tft->drawString("tap to back", 232, BAT_HDR_H / 2);

  // ── Battery section ────────────────────────────────────────────────────────
  batSection("Battery", 30);

  bool  batConn  = ttgo->power->isBatteryConnect();
  int   pct      = batConn ? (int)ttgo->power->getBattPercentage() : 0;
  float batVolt  = batConn ? ttgo->power->getBattVoltage()         : 0;
  float batCur   = ttgo->power->isChargeing()
                     ?  ttgo->power->getBattChargeCurrent()
                     : -ttgo->power->getBattDischargeCurrent();
  bool  charging = ttgo->power->isChargeing();

  // Connected
  batRow("Connected",
         batConn ? "Yes" : "No",
         54,
         batConn ? TFT_GREEN : TFT_RED);

  // Charge %
  {
    char buf[12];
    snprintf(buf, sizeof(buf), "%d %%", pct);
    uint32_t col = pct > 50 ? TFT_GREEN : (pct > 20 ? TFT_YELLOW : TFT_RED);
    batRow("Charge", buf, 75, col);
  }

  // Voltage
  {
    char buf[14];
    snprintf(buf, sizeof(buf), "%d mV", (int)batVolt);
    batRow("Voltage", buf, 96);
  }

  // Status
  batRow("Status",
         charging ? "Charging" : "Discharging",
         117,
         charging ? TFT_GREEN : TFT_WHITE);

  // Current
  {
    char buf[14];
    snprintf(buf, sizeof(buf), "%d mA", (int)fabsf(batCur));
    batRow("Current", buf, 138);
  }

  // ── USB section — only shown when USB is present ───────────────────────────
  bool usbIn = ttgo->power->isVBUSPlug();
  if (!usbIn) return;

  batSection("USB", 155);

  float usbVolt = ttgo->power->getVbusVoltage();
  float usbCur  = ttgo->power->getVbusCurrent();

  batRow("Connected",   "Yes",  175, TFT_GREEN);

  {
    char buf[14];
    snprintf(buf, sizeof(buf), "%d mV", (int)usbVolt);
    batRow("USB Voltage", buf, 196);
  }
  {
    char buf[14];
    snprintf(buf, sizeof(buf), "%d mA", (int)usbCur);
    batRow("USB Current", buf, 217);
  }
}
