#pragma once
#include <FastLED.h>

void setupDisplay();
void loopDisplay();
void loopBrightness();
void loopAutoRotate();
void drawBootScreen();
void drawUpdate();
void drawProgress(int barW);
void drawDone();
void drawError();
void drawChar(int x, int y, char c, CRGB color);
void setLED(int x, int y, CRGB color);
void _gifCloseIfOpen();
const char* getScreenName();
void screensaverApplyBrightness();
void resetScreensaverIdle();
void drawIndicators();
