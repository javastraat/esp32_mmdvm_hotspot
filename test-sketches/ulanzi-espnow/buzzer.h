#pragma once
#include <Arduino.h>

void setupBuzzer();
void loopBuzzer();
void buzzerBeep();
void buzzerClick();
void buzzerPlay(uint16_t freq, uint16_t durationMs, uint8_t duty);
uint8_t buzzerVolToDuty(uint8_t vol);
