// buzzer.cpp — Non-blocking LEDC buzzer engine and boot chime.
#include "buzzer.h"
#include "globals.h"

uint8_t buzzerVolToDuty(uint8_t vol) {
  uint8_t d = (uint8_t)map(vol, 0, 255, 0, 100);
  return (d < 1) ? 1 : d;
}

// Queue a tone — safe to call from any context (incl. ESP-NOW callback)
void buzzerPlay(uint16_t freq, uint16_t durationMs, uint8_t duty) {
  if (duty == 0) return;
  buzzerQFreq     = freq;
  buzzerQDuration = durationMs;
  buzzerQDuty     = duty;
  buzzerQPending  = true;
}

void buzzerBeep() {
  if (!buzzerPocsagEnabled) return;
  buzzerPlay(BUZZER_FREQ_BEEP, BUZZER_DUR_BEEP_MS, buzzerVolToDuty(buzzerPocsagVolume));
}

void buzzerClick() {
  if (!buzzerClickEnabled) return;
  buzzerPlay(BUZZER_FREQ_CLICK, BUZZER_DUR_CLICK_MS, buzzerVolToDuty(buzzerClickVolume));
}

void loopBuzzer() {
  if (buzzerQPending) {
    buzzerQPending = false;
    ledcChangeFrequency(BUZZER_PIN, buzzerQFreq, BUZZER_LEDC_RES);
    ledcWrite(BUZZER_PIN, buzzerQDuty);
    buzzerEndMs = millis() + buzzerQDuration;
  }
  if (buzzerEndMs > 0 && millis() >= buzzerEndMs) {
    ledcWrite(BUZZER_PIN, 0);
    buzzerEndMs = 0;
  }
}

void setupBuzzer() {
  // ledcAttach reconfigures GPIO15 as LEDC output (overrides INPUT_PULLDOWN)
  ledcAttach(BUZZER_PIN, BUZZER_FREQ_BEEP, BUZZER_LEDC_RES);
  ledcWrite(BUZZER_PIN, 0);
  if (buzzerBootEnabled) {
    uint8_t duty = buzzerVolToDuty(buzzerBootVolume);
    ledcWrite(BUZZER_PIN, duty);
    delay(BUZZER_DUR_BEEP_MS);
    ledcWrite(BUZZER_PIN, 0);
  }
}
