/*
 * system_modem.cpp - MMDVM Modem Management Implementation
 */

#include "system/system_modem.h"
#include "system/system_firmware.h"
#include "system/system_logger.h"
#include "system/system_mqtt.h"
#include "include/config.h"
#include "mmdvm/mmdvm_pocsag.h"
#include "mmdvm/mmdvm_dmr.h"
#include <Preferences.h>

// Global modem state
bool mmdvmReady = false;
bool mmdvmWakeupActive = false;
TaskHandle_t modemTaskHandle = NULL;

// Set to true by the /api/test-cwid handler to trigger an immediate CW ID.
volatile bool cwidTestRequested = false;

// Extern - defined in system_firmware.cpp
extern String mqttModemTaskTopic;

// Extern - defined in esp32-rtos-mmdvm.ino
extern Preferences preferences;

// Wakeup serial - MUST stay active to keep modem awake
HardwareSerial MMDVMWakeup(1);

// External settings (from main .ino)
extern uint32_t dmrRxFreq;
extern uint32_t dmrTxFreq;
extern uint8_t dmrColorCode;
extern uint8_t dmrRfPower;
extern bool modeDmrEnabled;
extern bool modeDstarEnabled;
extern bool modeYsfEnabled;
extern bool modeP25Enabled;
extern bool modeNxdnEnabled;
extern bool modePocsagEnabled;
extern uint32_t pocsagFrequency;
extern String userCallsign;

void initModemTask()
{
  BaseType_t result = xTaskCreatePinnedToCore(
      modemTask,
      "MODEM Task",
      MODEM_TASK_STACK,
      NULL,
      MODEM_TASK_PRIORITY,
      &modemTaskHandle,
      1 // Core 1
  );
  if (result != pdPASS)
    log_e("[Modem] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());
}

/*
 * Send MMDVM command with data
 */
void sendMMDVMCommand(uint8_t command, uint8_t *data, uint8_t dataLength)
{
  if (dataLength > 257) {
    log_e("[MODEM] sendMMDVMCommand: payload too large (%u bytes), dropping", dataLength);
    return;
  }
  uint8_t buffer[260];
  buffer[0] = MMDVM_FRAME_START;
  buffer[1] = dataLength + 3; // Length includes: length byte, command byte, data
  buffer[2] = command;

  if (data != NULL && dataLength > 0)
  {
    memcpy(buffer + 3, data, dataLength);
  }

  MMDVM_SERIAL.write(buffer, dataLength + 3);
}

/*
 * Send frequency configuration to modem
 * Format matches MMDVM_HS firmware expectation:
 *   Byte 0: Mode/flags (0x00)
 *   Bytes 1-4: RX frequency (little-endian)
 *   Bytes 5-8: TX frequency (little-endian)
 *   Byte 9: RF power level
 *   Bytes 10-13: POCSAG frequency (little-endian, 0 if unused)
 */
void sendFrequency(uint32_t rxFreq, uint32_t txFreq, uint8_t rfPower)
{
  uint8_t freqData[14];
  memset(freqData, 0, sizeof(freqData));

  // Byte 0: Mode/flags
  freqData[0] = 0x00;

  // RX Frequency (4 bytes, little-endian)
  freqData[1] = (rxFreq >> 0) & 0xFF;
  freqData[2] = (rxFreq >> 8) & 0xFF;
  freqData[3] = (rxFreq >> 16) & 0xFF;
  freqData[4] = (rxFreq >> 24) & 0xFF;

  // TX Frequency (4 bytes, little-endian)
  freqData[5] = (txFreq >> 0) & 0xFF;
  freqData[6] = (txFreq >> 8) & 0xFF;
  freqData[7] = (txFreq >> 16) & 0xFF;
  freqData[8] = (txFreq >> 24) & 0xFF;

  // RF Power level
  freqData[9] = rfPower;

  // Bytes 10-13: POCSAG TX frequency.
  // MUST be a valid frequency - the modem firmware rejects the entire CMD_SET_FREQ
  // (including the RX/TX frequencies) if this field fails the range check.
  extern uint32_t pocsagFrequency;
  uint32_t pocsagFreq = pocsagFrequency;
  freqData[10] = (pocsagFreq >>  0) & 0xFF;
  freqData[11] = (pocsagFreq >>  8) & 0xFF;
  freqData[12] = (pocsagFreq >> 16) & 0xFF;
  freqData[13] = (pocsagFreq >> 24) & 0xFF;

  sendMMDVMCommand(CMD_SET_FREQ, freqData, 14);
}

/*
 * Wait for ACK response from modem.
 *
 * Properly parses the MMDVM framing protocol so that unsolicited frames
 * (e.g. status updates) arriving before the ACK are skipped rather than
 * preventing the ACK from ever being detected.
 *
 * Frame layout: [0xE0 | len | cmd | data...]
 *   - len = total frame bytes (includes 0xE0, len, cmd, data)
 *   - ACK frame: [0xE0, 0x03, 0x70]
 *   - NAK frame: [0xE0, 0x04, 0x7F, reason]
 *
 * Bug in old code: rxBuffer[2] was set once by the first frame received and
 * never re-evaluated for subsequent frames, so an intervening status frame
 * would permanently shadow the real ACK.
 */
bool waitForModemAck(unsigned long timeout)
{
  unsigned long startTime = millis();
  uint8_t rxBuffer[32];
  uint8_t rxPtr = 0;

  while (millis() - startTime < timeout)
  {
    if (MMDVM_SERIAL.available())
    {
      uint8_t byte = MMDVM_SERIAL.read();

      // Wait for frame-start byte before collecting
      if (rxPtr == 0 && byte != MMDVM_FRAME_START)
        continue;

      if (rxPtr < sizeof(rxBuffer))
        rxBuffer[rxPtr++] = byte;

      // Need at least 3 bytes to know the command
      if (rxPtr < 3)
        continue;

      // Once we have the length byte we know when the frame is complete
      uint8_t frameLen = rxBuffer[1];
      if (frameLen < 3 || frameLen > sizeof(rxBuffer))
      {
        // Malformed frame — reset and re-sync
        rxPtr = 0;
        continue;
      }

      if (rxPtr >= frameLen)
      {
        // Complete frame received — inspect command byte
        uint8_t cmd = rxBuffer[2];
        if (cmd == CMD_ACK)
          return true;

        if (cmd == CMD_NAK)
        {
          addLogMessage("[MODEM Task] NAK received");
          publishMqtt(mqttModemTaskTopic.c_str(), "{\"event\":\"nak_received\"}");
          return false;
        }

        // Any other frame (status, heartbeat, etc.) — skip it and look
        // for the next frame start.
        rxPtr = 0;
      }
    }
    else
    {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }

  return false;
}

/*
 * Initialize and configure MMDVM modem
 * Based on esp32_mmdvm_hotspot setupMMDVM()
 */
void setupMMDVM()
{
  // Always reset so the post-init GET_VERSION retry runs fresh.
  // The NVS-cached value is only for display before the modem responds on boot;
  // once setupMMDVM() is called we always re-read directly from the hardware.
  modemFirmwareVersion = "Unknown";

  addLogMessage("[MODEM Task] Initializing MMDVM...");
  publishMqtt(mqttModemTaskTopic.c_str(), "{\"event\":\"initializing\"}");

  // CRITICAL: Initialize main MMDVM serial FIRST (per working hotspot code)
  MMDVM_SERIAL.begin(MMDVM_SERIAL_BAUD, SERIAL_8N1, MMDVM_RX_PIN, MMDVM_TX_PIN);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  addLogMessage("[MODEM Task] Serial2 initialized on GPIO " + String(MMDVM_TX_PIN) + "/" + String(MMDVM_RX_PIN));
  publishMqtt(mqttModemTaskTopic.c_str(),
              "{\"event\":\"serial_ready\",\"tx_pin\":" + String(MMDVM_TX_PIN) +
              ",\"rx_pin\":" + String(MMDVM_RX_PIN) + "}");

  // Now start wakeup serial on GPIO 13 and KEEP IT ACTIVE
  // The modem needs continuous UART activity to stay awake
  addLogMessage("[MODEM Task] Starting MMDVM wakeup serial on GPIO " + String(MMDVM_WAKEUP_PIN) + "...");
  publishMqtt(mqttModemTaskTopic.c_str(),
              "{\"event\":\"wakeup_serial_starting\",\"pin\":" + String(MMDVM_WAKEUP_PIN) + "}");
  MMDVMWakeup.begin(115200, SERIAL_8N1, 1, MMDVM_WAKEUP_PIN); // RX=1, TX=GPIO 13
  mmdvmWakeupActive = true;
  vTaskDelay(100 / portTICK_PERIOD_MS);

  // Send wakeup commands and try to get version
  uint8_t wakeCmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};
  bool versionFromWakeup = false;
  uint8_t wakeRxBuffer[256];
  int wakeRxPtr = 0;

  addLogMessage("[MODEM Task] Sending wakeup commands...");
  publishMqtt(mqttModemTaskTopic.c_str(), "{\"event\":\"sending_wakeup\"}");
  for (int attempt = 0; attempt < 20 && !versionFromWakeup; attempt++)
  {
    // Send on wakeup serial to keep modem active
    MMDVMWakeup.write(wakeCmd, 3);
    MMDVMWakeup.flush();
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Also try on main serial occasionally
    if (attempt % 5 == 0)
    {
      MMDVM_SERIAL.write(wakeCmd, 3);
      MMDVM_SERIAL.flush();
    }

    // Check for response on wakeup serial
    unsigned long timeout = millis() + 50;
    while (millis() < timeout && MMDVMWakeup.available())
    {
      uint8_t byte = MMDVMWakeup.read();

      if (wakeRxPtr == 0 && byte != MMDVM_FRAME_START)
      {
        continue;
      }

      wakeRxBuffer[wakeRxPtr++] = byte;

      if (wakeRxPtr >= 2)
      {
        uint8_t frameLength = wakeRxBuffer[1];

        if (wakeRxPtr >= frameLength)
        {
          if (frameLength >= 3 && wakeRxBuffer[2] == CMD_GET_VERSION && wakeRxPtr > 4)
          {
            modemFirmwareVersion = "Unknown";
            for (int j = 4; j < wakeRxPtr && wakeRxBuffer[j] != 0x00; j++)
            {
              if (wakeRxBuffer[j] >= 32 && wakeRxBuffer[j] < 127)
              {
                modemFirmwareVersion += (char)wakeRxBuffer[j];
              }
            }
            addLogMessage("[MODEM Task] Modem Firmware (from wakeup): " + modemFirmwareVersion);
            publishMqtt(mqttModemTaskTopic.c_str(),
                        "{\"event\":\"firmware_version\",\"source\":\"wakeup\",\"version\":\"" + modemFirmwareVersion + "\"}");
            versionFromWakeup = true;
          }
          wakeRxPtr = 0;
        }
      }

      if (wakeRxPtr >= sizeof(wakeRxBuffer))
      {
        wakeRxPtr = 0;
      }
    }
  }

  addLogMessage("[MODEM Task] MMDVM wakeup active (SVC LED should be blinking)");
  publishMqtt(mqttModemTaskTopic.c_str(), "{\"event\":\"wakeup_active\"}");

  // Clear any stale data from wakeup phase
  while (MMDVM_SERIAL.available())
  {
    MMDVM_SERIAL.read();
  }

  // Now modem is awake - send GET_VERSION on main serial and wait for response
  if (!versionFromWakeup)
  {
    uint8_t mainRxBuffer[256];
    int mainRxPtr = 0;

    for (int vAttempt = 0; vAttempt < 5 && modemFirmwareVersion.isEmpty(); vAttempt++)
    {
      // Clear buffer
      while (MMDVM_SERIAL.available()) MMDVM_SERIAL.read();
      mainRxPtr = 0;

      // Send GET_VERSION on main serial
      MMDVM_SERIAL.write(wakeCmd, 3);
      MMDVM_SERIAL.flush();

      // Wait for response with longer timeout
      unsigned long mainTimeout = millis() + 500;
      while (millis() < mainTimeout)
      {
        if (MMDVM_SERIAL.available())
        {
          uint8_t byte = MMDVM_SERIAL.read();
          if (mainRxPtr == 0 && byte != MMDVM_FRAME_START)
          {
            continue;
          }
          mainRxBuffer[mainRxPtr++] = byte;
          if (mainRxPtr >= 2)
          {
            uint8_t frameLength = mainRxBuffer[1];
            if (mainRxPtr >= frameLength)
            {
              if (frameLength >= 3 && mainRxBuffer[2] == CMD_GET_VERSION && mainRxPtr > 4)
              {
                modemFirmwareVersion = "";
                for (int j = 4; j < mainRxPtr && mainRxBuffer[j] != 0x00; j++)
                {
                  if (mainRxBuffer[j] >= 32 && mainRxBuffer[j] < 127)
                  {
                    modemFirmwareVersion += (char)mainRxBuffer[j];
                  }
                }
                addLogMessage("[MODEM Task] Modem Firmware: " + modemFirmwareVersion);
                publishMqtt(mqttModemTaskTopic.c_str(),
                            "{\"event\":\"firmware_version\",\"source\":\"main\",\"version\":\"" + modemFirmwareVersion + "\"}");
                break;
              }
              mainRxPtr = 0;
            }
          }
          if (mainRxPtr >= sizeof(mainRxBuffer))
          {
            mainRxPtr = 0;
          }
        }
        else
        {
          vTaskDelay(10 / portTICK_PERIOD_MS);
        }
      }
    }
  }

  vTaskDelay(500 / portTICK_PERIOD_MS);

  // Only request firmware version ONCE, before config. Do not repeat after config.
  // (No code here: removed duplicate version request)

  // Set frequency BEFORE config (per MMDVMHost sequence)
  addLogMessage("[MODEM Task] Setting frequency: RX=" + String(dmrRxFreq) + " TX=" + String(dmrTxFreq) + ", Power=" + String(dmrRfPower) + ", CC=" + String(dmrColorCode));
  publishMqtt(mqttModemTaskTopic.c_str(),
              "{\"event\":\"set_frequency\",\"rx_hz\":" + String(dmrRxFreq) +
              ",\"tx_hz\":" + String(dmrTxFreq) +
              ",\"power\":" + String(dmrRfPower) +
              ",\"color_code\":" + String(dmrColorCode) + "}");
  sendFrequency(dmrRxFreq, dmrTxFreq, dmrRfPower);
  vTaskDelay(100 / portTICK_PERIOD_MS);

  // Build configuration packet (23 bytes)
  uint8_t config[23];
  memset(config, 0, sizeof(config));

  // Byte 0: Flags (bit 7=simplex)
  config[0] = 0x80; // Simplex mode

  // Byte 1: Mode enables (bitfield)
  uint8_t modeEnables = 0x00;
  if (modeDstarEnabled)
    modeEnables |= 0x01;
  if (modeDmrEnabled)
    modeEnables |= 0x02;
  if (modeYsfEnabled)
    modeEnables |= 0x04;
  if (modeP25Enabled)
    modeEnables |= 0x08;
  if (modeNxdnEnabled)
    modeEnables |= 0x10;
  if (modePocsagEnabled)
    modeEnables |= 0x20;
  config[1] = modeEnables;

  // Byte 2: TX delay (0-50)
  config[2] = MMDVM_TX_DELAY;

  // Byte 3: Modem state (set to first enabled mode, or IDLE)
  uint8_t modemState = 0x00; // IDLE
  String modeStr = "IDLE";
  if (modeDmrEnabled)
  {
    modemState = 0x02; // DMR
    modeStr = "DMR";
  }
  else if (modeDstarEnabled)
  {
    modemState = 0x01; // D-Star
    modeStr = "D-Star";
  }
  else if (modeYsfEnabled)
  {
    modemState = 0x03; // YSF
    modeStr = "YSF";
  }
  else if (modeP25Enabled)
  {
    modemState = 0x04; // P25
    modeStr = "P25";
  }
  else if (modeNxdnEnabled)
  {
    modemState = 0x05; // NXDN
    modeStr = "NXDN";
  }
  else if (modePocsagEnabled)
  {
    modemState = 0x06; // POCSAG (STATE_POCSAG = 6)
    modeStr = "POCSAG";
  }
  config[3] = modemState;

  // Byte 4: RX level (set to 0xFF for max)
  config[4] = 0xFF;

  // Byte 5: CW ID TX level (set to 0xFF for max)
  config[5] = 0xFF;

  // Byte 6: DMR Color Code (0-15) - CRITICAL for DMR
  config[6] = dmrColorCode;

  // Byte 7: DMR delay (duplex only, 0 for simplex)
  config[7] = 0x00;

  // Set all unused bytes to 0x00 for compatibility
  config[8] = 0x00;
  config[13] = 0x00;
  config[14] = 0x00;
  config[16] = 0x00;
  config[18] = 0x00;
  config[19] = 0x00;
  config[20] = 0x00;
  config[22] = 0x00;

  // Bytes 9-21: TX levels for each mode (set to 0xFF for max)
  config[9] = 0xFF;   // D-Star
  config[10] = 0xFF;  // DMR
  config[11] = 0xFF;  // YSF
  config[12] = 0xFF;  // P25
  config[15] = 0xFF;  // NXDN
  config[17] = 0xFF;  // POCSAG
  config[21] = 0xFF;  // M17

  addLogMessage("[MODEM Task] Setting configuration - Mode: " + modeStr + ", Color Code: " + String(dmrColorCode));
  publishMqtt(mqttModemTaskTopic.c_str(),
              "{\"event\":\"set_config\",\"mode\":\"" + modeStr +
              "\",\"color_code\":" + String(dmrColorCode) + "}");
  sendMMDVMCommand(CMD_SET_CONFIG, config, 23);
  vTaskDelay(400 / portTICK_PERIOD_MS); // Increased delay for modem to apply config

  // Post-init firmware version retry.
  // The modem is now confirmed responding (it ACK'd SET_FREQ and SET_CONFIG).
  // Try GET_VERSION one more time with a clean buffer before declaring ready.
  if (modemFirmwareVersion.isEmpty() || modemFirmwareVersion == "Unknown")
  {
    uint8_t piRxBuf[256];
    int     piPtr = 0;
    uint8_t getVerCmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};

    while (MMDVM_SERIAL.available()) MMDVM_SERIAL.read(); // flush stale bytes
    MMDVM_SERIAL.write(getVerCmd, 3);
    MMDVM_SERIAL.flush();

    unsigned long piEnd = millis() + 1000;
    while (millis() < piEnd)
    {
      if (MMDVM_SERIAL.available())
      {
        uint8_t b = MMDVM_SERIAL.read();
        if (piPtr == 0 && b != MMDVM_FRAME_START) continue;
        piRxBuf[piPtr++] = b;
        if (piPtr >= 2)
        {
          uint8_t flen = piRxBuf[1];
          if (piPtr >= flen)
          {
            if (flen >= 3 && piRxBuf[2] == CMD_GET_VERSION && piPtr > 4)
            {
              modemFirmwareVersion = "";
              for (int j = 4; j < piPtr && piRxBuf[j] != 0x00; j++)
              {
                if (piRxBuf[j] >= 32 && piRxBuf[j] < 127)
                  modemFirmwareVersion += (char)piRxBuf[j];
              }
              addLogMessage("[MODEM Task] Modem Firmware (post-init): " + modemFirmwareVersion);
              publishMqtt(mqttModemTaskTopic.c_str(),
                          "{\"event\":\"firmware_version\",\"source\":\"post_init\",\"version\":\"" +
                          modemFirmwareVersion + "\"}");
              break;
            }
            piPtr = 0; // not a version frame, skip and re-sync
          }
        }
        if (piPtr >= (int)sizeof(piRxBuf)) piPtr = 0;
      }
      else
      {
        vTaskDelay(10 / portTICK_PERIOD_MS);
      }
    }
  }

  // Persist detected firmware version to NVS so the firmware page can show it
  // immediately on the next boot before the modem has responded.
  if (modemFirmwareVersion.length() > 0 && modemFirmwareVersion != "Unknown")
  {
    if (preferences.getString("modem_fw", "") != modemFirmwareVersion)
      preferences.putString("modem_fw", modemFirmwareVersion.c_str());

    // Parse the version string into individual fields for MQTT / UI use.
    parseModemFirmwareVersion();
  }

  // Mark modem as ready (even without version, commands are working)
  // The modem responds to frequency and config commands even if version read fails
  if (modemFirmwareVersion.length() > 0 && modemFirmwareVersion != "Unknown")
  {
    mmdvmReady = true;
    addLogMessage("[MODEM Task] MMDVM initialized successfully - Mode: " + modeStr + ", FW: " + modemFirmwareVersion);
    publishMqtt(mqttModemTaskTopic.c_str(),
                "{\"event\":\"initialized\",\"firmware\":\"" + modemFirmwareVersion +
                "\",\"mode\":\"" + modeStr +
                "\",\"color_code\":" + String(dmrColorCode) + "}");
  }
  else
  {
    // Modem is responding to commands (frequency/config) even without version
    mmdvmReady = true;
    addLogMessage("[MODEM Task] MMDVM initialized (no firmware version detected, but modem is responding) - Mode: " + modeStr);
    publishMqtt(mqttModemTaskTopic.c_str(),
                "{\"event\":\"initialized\",\"firmware\":\"unknown\",\"mode\":\"" + modeStr +
                "\",\"color_code\":" + String(dmrColorCode) + "}");
  }
}

/*
 * Restore modem to DMR/idle configuration after CW ID transmission.
 * Mirrors restoreModemAfterPocsag() in mmdvm_pocsag.cpp.
 */
static void restoreModemAfterCwid()
{
  while (MMDVM_SERIAL.available()) MMDVM_SERIAL.read();
  sendFrequency(dmrRxFreq, dmrTxFreq, dmrRfPower);
  waitForModemAck(1000);
  vTaskDelay(50 / portTICK_PERIOD_MS);

  uint8_t config[23];
  memset(config, 0, sizeof(config));
  config[0]  = 0x80; // simplex
  uint8_t modeEnables = 0x00;
  if (modeDstarEnabled)  modeEnables |= 0x01;
  if (modeDmrEnabled)    modeEnables |= 0x02;
  if (modeYsfEnabled)    modeEnables |= 0x04;
  if (modeP25Enabled)    modeEnables |= 0x08;
  if (modeNxdnEnabled)   modeEnables |= 0x10;
  if (modePocsagEnabled) modeEnables |= 0x20;
  config[1]  = modeEnables;
  config[2]  = MMDVM_TX_DELAY;
  config[3]  = MODE_IDLE;
  config[4]  = 0xFF; config[5]  = 0xFF;
  config[6]  = dmrColorCode;
  config[9]  = 0xFF; config[10] = 0xFF; config[11] = 0xFF;
  config[12] = 0xFF; config[15] = 0xFF; config[17] = 0xFF;
  config[21] = 0xFF;
  while (MMDVM_SERIAL.available()) MMDVM_SERIAL.read();
  sendMMDVMCommand(CMD_SET_CONFIG, config, 23);
  waitForModemAck(1000);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  addLogMessage("[MODEM Task] Modem restored after CW ID");
}

/*
 * TASK: MMDVM Modem Management
 *
 * Manages modem initialization, keeps wakeup serial active,
 * and handles periodic CW ID transmission for regulatory compliance.
 */
void modemTask(void *parameter)
{
  addLogMessage("[MODEM Task] Started");
  publishMqtt(mqttModemTaskTopic.c_str(), "{\"event\":\"started\"}");

  setupMMDVM();

  // Start timer after setup so first CW ID fires after one full interval,
  // not immediately on boot.
  unsigned long lastCwidMs = millis();

  // MQTT is typically not connected yet when setupMMDVM() runs during boot.
  // Track whether we have successfully published the firmware version so we
  // can retry once MQTT comes online.
  bool firmwareVersionPublished = false;

  while (true)
  {
    // Deferred firmware version publish: retry until MQTT is connected.
    if (!firmwareVersionPublished && mqttConnected &&
        modemFirmwareVersion.length() > 0 && modemFirmwareVersion != "Unknown")
    {
      publishMqtt(mqttModemTaskTopic.c_str(),
                  String("{\"event\":\"firmware_version\"") +
                 // ",\"full\":\""     + modemFirmwareVersion + "\"" +
                  ",\"hardware\":\"" + modemFwHardware      + "\"" +
                  ",\"version\":\""  + modemFwVersion       + "\"" +
                  ",\"date\":\""     + modemFwDate          + "\"" +
                  ",\"crystal\":\""  + modemFwCrystal       + "\"" +
                  ",\"chip\":\""     + modemFwChip          + "\"" +
                  ",\"authors\":\""  + modemFwAuthors       + "\"" +
                  ",\"git_id\":\""   + modemFwGitId         + "\"}");
      firmwareVersionPublished = true;
    }

    if ((cwidEnabled || cwidTestRequested) && mmdvmReady && !pocsagTxInProgress)
    {
      // Guard against cwidIntervalMin == 0 which would make interval = 0 ms
      // and fire CW ID on every loop iteration.
      uint8_t safeInterval = (cwidIntervalMin == 0) ? 1 : cwidIntervalMin;
      unsigned long cwidIntervalMs = (unsigned long)safeInterval * 60UL * 1000UL;
      bool isTestTx = cwidTestRequested;
      if (isTestTx || millis() - lastCwidMs >= cwidIntervalMs)
      {
        cwidTestRequested = false;

        // Wait for any ongoing DMR TX or RF RX to finish before keying up
        if (dmrTxActive || dmrRfRxActive)
        {
          addLogMessage("[MODEM Task] CW ID: waiting for DMR to finish...");
          while (dmrTxActive || dmrRfRxActive)
            vTaskDelay(500 / portTICK_PERIOD_MS);
          vTaskDelay(500 / portTICK_PERIOD_MS); // brief guard time after DMR ends
        }

        // Gate DMR serial reads for the duration of CW ID
        pocsagTxInProgress = true;

        // Choose frequency: if any digital mode is active use DMR freq;
        // if POCSAG-only, transmit on the POCSAG frequency so the ID
        // appears on the same channel as the paging transmissions.
        bool anyDigitalMode = modeDmrEnabled || modeDstarEnabled ||
                              modeYsfEnabled || modeP25Enabled || modeNxdnEnabled;
        uint32_t cwidFreq = (modePocsagEnabled && !anyDigitalMode)
                            ? pocsagFrequency
                            : dmrTxFreq;

        // Flush stale bytes, then set modem IDLE so firmware accepts SEND_CWID
        // (firmware rejects SEND_CWID unless m_modemState == STATE_IDLE)
        while (MMDVM_SERIAL.available()) MMDVM_SERIAL.read();
        uint8_t idleMode = MODE_IDLE;
        sendMMDVMCommand(CMD_SET_MODE, &idleMode, 1);
        waitForModemAck(1000);
        vTaskDelay(50 / portTICK_PERIOD_MS);

        // If CW ID frequency differs from current modem frequency, retune first
        if (cwidFreq != dmrTxFreq)
        {
          while (MMDVM_SERIAL.available()) MMDVM_SERIAL.read();
          sendFrequency(cwidFreq, cwidFreq, dmrRfPower);
          waitForModemAck(1000);
          vTaskDelay(50 / portTICK_PERIOD_MS);
        }

        // Send CW ID callsign
        while (MMDVM_SERIAL.available()) MMDVM_SERIAL.read();
        sendMMDVMCommand(CMD_SEND_CWID,
                         (uint8_t *)userCallsign.c_str(),
                         (uint8_t)userCallsign.length());

        // Always wait the full TX time regardless of ACK detection.
        // The firmware processes SEND_CWID and starts transmitting before (or
        // simultaneously with) sending the ACK.  If we abort early on an ACK
        // timeout, restoreModemAfterCwid() fires immediately and cuts the CW
        // off mid-callsign.  Waiting first guarantees the full callsign is sent.
        // At ~20 WPM: ~600 ms per character + 3 s end margin.
        unsigned long txWaitMs = (unsigned long)userCallsign.length() * 600UL + 3000UL;

        waitForModemAck(1000); // consume ACK/NAK if it arrives; result not critical
        String txReason = isTestTx ? " [TEST]" : "";
        addLogMessage("[MODEM Task] CW ID TX" + txReason + ": " + userCallsign +
                      " (on " + String(cwidFreq / 1000) + " kHz)");
        publishMqtt(mqttModemTaskTopic.c_str(),
                    "{\"event\":\"cwid_tx\",\"callsign\":\"" + userCallsign +
                    "\",\"freq_khz\":" + String(cwidFreq / 1000) +
                    ",\"test\":" + String(isTestTx ? "true" : "false") + "}");
        vTaskDelay(txWaitMs / portTICK_PERIOD_MS);

        restoreModemAfterCwid();
        pocsagTxInProgress = false;
        lastCwidMs = millis();
      }
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
