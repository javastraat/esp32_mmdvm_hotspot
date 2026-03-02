
/*
 * MMDVM POCSAG Protocol Module Implementation
 * Based on MMDVMHost POCSAGControl.cpp protocol
 */

#include "include/config.h"
#include "system/system_logger.h"
#include "system/system_modem.h"
#include "system/system_oled.h"
#include "mmdvm/mmdvm_pocsag.h"
#include "mmdvm/mmdvm_dmr.h"
#include <WiFi.h>
#include "system/system_eth.h"
#include <vector>
#include <cstdio>
#include "system/service_mqtt.h"
#include "system/service_telegram.h"

// External references to runtime settings (defined in main .ino)
extern String mqttPocsagTaskTopic;

// Whitelist/Blacklist
extern String pocsagBlacklist;
extern String pocsagWhitelist;

// External settings for restoring modem after POCSAG
extern uint32_t dmrRxFreq;
extern uint32_t dmrTxFreq;
extern uint8_t dmrRfPower;
extern uint8_t dmrColorCode;
extern bool modeDstarEnabled, modeDmrEnabled, modeYsfEnabled;
extern bool modeP25Enabled, modeNxdnEnabled, modePocsagEnabled;

// POCSAG protocol constants (from MMDVMHost POCSAGDefines.h)
#define POCSAG_SYNC_WORD        0x7CD215D8U
#define POCSAG_IDLE_WORD        0x7A89C197U
#define POCSAG_FRAME_WORDS      17U   // 1 sync + 16 data words
#define POCSAG_FRAME_BYTES      68U   // 17 * 4
#define POCSAG_FRAME_ADDRESSES  8U    // 8 address slots per batch

// Functional codes (defined in mmdvm_pocsag.h, mirrored here for internal use)
#ifndef FUNCTIONAL_NUMERIC
#define FUNCTIONAL_NUMERIC       0U
#define FUNCTIONAL_ALERT1        1U
#define FUNCTIONAL_ALERT2        2U
#define FUNCTIONAL_ALPHANUMERIC  3U
#endif

// --- BCH and parity (from MMDVMHost POCSAGControl.cpp) ---
static void addBCHAndParity(uint32_t &word)


{
  uint32_t temp = word;
  for (unsigned int i = 0U; i < 21U; i++, temp <<= 1) {
    if (temp & 0x80000000U)
      temp ^= 0xED200000U;
  }
  word |= (temp >> 21);

  temp = word;
  unsigned int parity = 0U;
  for (unsigned int i = 0U; i < 32U; i++, temp <<= 1) {
    if (temp & 0x80000000U)
      parity++;
  }
  if ((parity % 2U) == 1U)
    word |= 0x00000001U;
}

// --- Build address word for a given RIC and functional code ---
static uint32_t makeAddressWord(uint32_t ric, uint8_t functional)
{
  uint32_t word = 0x00000000U;

  switch (functional) {
    case FUNCTIONAL_ALPHANUMERIC: word = 0x00001800U; break;
    case FUNCTIONAL_ALERT1:      word = 0x00000800U; break;
    case FUNCTIONAL_ALERT2:      word = 0x00001000U; break;
    case FUNCTIONAL_NUMERIC:
    default: break;
  }

  word |= (ric / POCSAG_FRAME_ADDRESSES) << 13;
  addBCHAndParity(word);
  return word;
}

// --- Pack numeric message into 32-bit POCSAG data words (4 bits/digit, 5 per word) ---
// POCSAG numeric encoding uses bit-reversed BCD, packed MSB-first, 5 codes per 20-bit word
static std::vector<uint32_t> packNumeric(const String &message)
{
  // Bit-reversed BCD table: index = digit ('0'-'9'), value = 4-bit code
  static const uint8_t bcdTable[10] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9};

  std::vector<uint32_t> words;
  uint32_t word = 0x80000000U; // bit 31 = 1 marks data word
  unsigned int n = 0U;         // bits used in current word (max 20)

  size_t msgLen = message.length();
  if (msgLen > 40U) msgLen = 40U; // 5 digits/word × 8 words max per frame

  for (size_t i = 0U; i < msgLen; i++) {
    char c = message[i];
    uint8_t code;
    if (c >= '0' && c <= '9')      code = bcdTable[c - '0'];
    else if (c == ' ')              code = 0x05U; // space
    else if (c == '-')              code = 0x0BU; // dash
    else                            code = 0x05U; // unknown → space

    // Pack 4 bits MSB-first into bits 30..11 of the word
    for (int b = 3; b >= 0; b--) {
      if ((code >> b) & 0x01U)
        word |= (1UL << (30U - n));
      n++;
      if (n == 20U) {
        addBCHAndParity(word);
        words.push_back(word);
        word = 0x80000000U;
        n = 0U;
      }
    }
  }

  if (n > 0U) {
    addBCHAndParity(word);
    words.push_back(word);
  }

  return words;
}

// --- Pack ASCII message into 32-bit POCSAG data words (7 bits/char, LSB first) ---
static std::vector<uint32_t> packASCII(const String &message)
{
  std::vector<uint32_t> words;
  uint32_t word = 0x80000000U; // bit 31 = 1 marks data word
  unsigned int n = 0U;

  size_t msgLen = message.length();
  if (msgLen > 80U) msgLen = 80U;

  for (size_t i = 0U; i < msgLen; i++) {
    unsigned char c = message[i];
    for (unsigned int j = 0U; j < 7U; j++, c >>= 1) {
      if (c & 0x01U)
        word |= (1UL << (30U - n));
      n++;
      if (n == 20U) {
        addBCHAndParity(word);
        words.push_back(word);
        word = 0x80000000U;
        n = 0U;
      }
    }
  }

  // Flush remaining bits
  if (n > 0U) {
    addBCHAndParity(word);
    words.push_back(word);
  }

  return words;
}

// --- Helper: write a uint32_t to a byte buffer (big-endian) ---
static void wordToBytes(uint32_t word, uint8_t *buf)
{
  buf[0] = (word >> 24) & 0xFF;
  buf[1] = (word >> 16) & 0xFF;
  buf[2] = (word >>  8) & 0xFF;
  buf[3] = (word >>  0) & 0xFF;
}

// --- Set modem to POCSAG mode and configure frequency ---
void setModemPocsagMode()
{
  addLogMessage("[POCSAG] Setting modem to IDLE then setting POCSAG frequency");

  // Step 1: Force modem to IDLE so CMD_POCSAG_DATA will trigger
  // the modem's internal setMode(STATE_POCSAG) which configures the ADF7021
  // for POCSAG at the frequency we set below.
  while (MMDVM_SERIAL.available()) MMDVM_SERIAL.read(); // flush stale RX data
  uint8_t idleData[1] = { MODE_IDLE };
  sendMMDVMCommand(CMD_SET_MODE, idleData, 1);
  if (!waitForModemAck(1000)) {
    addLogMessage("[POCSAG] WARNING: No ACK for SET_MODE(IDLE)");
  }
  vTaskDelay(50 / portTICK_PERIOD_MS);

  // Step 2: Set POCSAG frequency BEFORE sending data so the ADF7021 is
  // tuned to the right channel when setMode(POCSAG) runs internally.
  // Send frequency with POCSAG frequency included
  extern uint32_t pocsagFrequency;
  uint32_t pocsagFreq = pocsagFrequency;
  addLogMessage(String("[POCSAG] Setting POCSAG frequency to ") + String(pocsagFreq) + " Hz");

  // Build frequency data: matches sendFrequency() format but with POCSAG freq
  // Byte 0: flags (0x00)
  // Bytes 1-4: RX freq (little-endian) - use POCSAG freq for both
  // Bytes 5-8: TX freq (little-endian) - use POCSAG freq
  // Byte 9: RF power level
  // Bytes 10-13: POCSAG freq (little-endian)
  uint8_t freqData[14];
  memset(freqData, 0, sizeof(freqData));
  freqData[0]  = 0x00;
  freqData[1]  = (pocsagFreq >>  0) & 0xFF;
  freqData[2]  = (pocsagFreq >>  8) & 0xFF;
  freqData[3]  = (pocsagFreq >> 16) & 0xFF;
  freqData[4]  = (pocsagFreq >> 24) & 0xFF;
  freqData[5]  = (pocsagFreq >>  0) & 0xFF;
  freqData[6]  = (pocsagFreq >>  8) & 0xFF;
  freqData[7]  = (pocsagFreq >> 16) & 0xFF;
  freqData[8]  = (pocsagFreq >> 24) & 0xFF;
  freqData[9]  = dmrRfPower; // RF power level (from configured value)
  freqData[10] = (pocsagFreq >>  0) & 0xFF;
  freqData[11] = (pocsagFreq >>  8) & 0xFF;
  freqData[12] = (pocsagFreq >> 16) & 0xFF;
  freqData[13] = (pocsagFreq >> 24) & 0xFF;

  while (MMDVM_SERIAL.available()) MMDVM_SERIAL.read(); // flush before SET_FREQ
  sendMMDVMCommand(CMD_SET_FREQ, freqData, 14);

  if (!waitForModemAck(1000)) {
    addLogMessage("[POCSAG] WARNING: No ACK for SET_FREQ(POCSAG)");
  } else {
    addLogMessage("[POCSAG] POCSAG frequency accepted by modem");
  }
  vTaskDelay(50 / portTICK_PERIOD_MS);

  // Step 3: Force m_pocsagEnable = true in the modem firmware.
  // CMD_POCSAG_DATA only writes to the TX buffer if m_pocsagEnable is set.
  // m_pocsagEnable is controlled by bit 5 of CMD_SET_CONFIG byte 1.
  // If modePocsagEnabled is false in NVS, setupMMDVM() sent bit5=0 and the modem
  // will silently drop CMD_POCSAG_DATA without NAK-ing.  Force it on here.
  uint8_t cfg[23];
  memset(cfg, 0, sizeof(cfg));
  cfg[0] = 0x80;  // simplex
  uint8_t modeEnables = 0x20;  // always enable POCSAG (bit 5)
  if (modeDstarEnabled) modeEnables |= 0x01;
  if (modeDmrEnabled)   modeEnables |= 0x02;
  if (modeYsfEnabled)   modeEnables |= 0x04;
  if (modeP25Enabled)   modeEnables |= 0x08;
  if (modeNxdnEnabled)  modeEnables |= 0x10;
  cfg[1]  = modeEnables;
  cfg[2]  = MMDVM_TX_DELAY;
  cfg[3]  = MODE_IDLE;       // modemState = IDLE (won't call io.ifConf, won't reset ADF7021)
  cfg[4]  = 0xFF;  cfg[5]  = 0xFF;
  cfg[6]  = dmrColorCode;
  cfg[9]  = 0xFF;  cfg[10] = 0xFF; cfg[11] = 0xFF;
  cfg[12] = 0xFF;  cfg[15] = 0xFF;
  cfg[17] = 128;   // POCSAG TX level (max)
  cfg[21] = 0xFF;
  while (MMDVM_SERIAL.available()) MMDVM_SERIAL.read(); // flush before SET_CONFIG
  sendMMDVMCommand(CMD_SET_CONFIG, cfg, 23);
  if (!waitForModemAck(1000)) {
    addLogMessage("[POCSAG] WARNING: No ACK for SET_CONFIG (POCSAG enable)");
  } else {
    addLogMessage("[POCSAG] POCSAG enabled in modem config");
  }
  vTaskDelay(50 / portTICK_PERIOD_MS);
}

// --- Build and send a single POCSAG frame (no modem setup/teardown) ---
static void sendPocsagFrame(uint32_t ric, const String &message, uint8_t functional)
{
  const char *encName = (functional == FUNCTIONAL_NUMERIC) ? "NUMERIC" : "ALPHANUMERIC";
  addLogMessage(String("[POCSAG] TX: RIC=") + String(ric) + " ENC=" + encName + " MSG='" + message + "'");
  String escapedMsg = message;
  escapedMsg.replace("\\", "\\\\");
  escapedMsg.replace("\"", "\\\"");
  String pocsagJson = String("{\"ric\":") + ric + ",\"functional\":" + functional + ",\"message\":\"" + escapedMsg + "\"}";
  publishMqtt(mqttPocsagTaskTopic.c_str(), pocsagJson);

  // Build address word with the requested functional code
  uint32_t addrWord = makeAddressWord(ric, functional);

  // Pack message into data words using the correct encoding
  std::vector<uint32_t> msgWords;
  if (functional == FUNCTIONAL_NUMERIC)
    msgWords = packNumeric(message);
  else
    msgWords = packASCII(message);

  // Build the complete 17-word POCSAG frame
  uint32_t frame[POCSAG_FRAME_WORDS];
  frame[0] = POCSAG_SYNC_WORD;
  for (unsigned int i = 1U; i < POCSAG_FRAME_WORDS; i++) {
    frame[i] = POCSAG_IDLE_WORD;
  }

  // Place address + message words at the correct slot
  unsigned int slot = ric % POCSAG_FRAME_ADDRESSES;
  unsigned int pos = slot * 2U + 1U;
  if (pos < POCSAG_FRAME_WORDS) {
    frame[pos] = addrWord;
    pos++;
  }
  for (size_t i = 0U; i < msgWords.size() && pos < POCSAG_FRAME_WORDS; i++) {
    frame[pos] = msgWords[i];
    pos++;
    if (i + 1 < msgWords.size() && pos >= POCSAG_FRAME_WORDS)
      addLogMessage("[POCSAG] Warning: message truncated — exceeded frame capacity");
  }

  // Convert frame words to bytes (big-endian)
  uint8_t frameBytes[POCSAG_FRAME_BYTES];
  for (unsigned int i = 0U; i < POCSAG_FRAME_WORDS; i++) {
    wordToBytes(frame[i], &frameBytes[i * 4]);
  }

  // Conditionally invert frame polarity
  if (POCSAG_INVERT_POLARITY) {
    for (unsigned int i = 0U; i < POCSAG_FRAME_BYTES; i++) {
      frameBytes[i] ^= 0xFF;
    }
  }

  // Reset modem to IDLE before every CMD_POCSAG_DATA.
  // The firmware's IO.process() calls io.ifConf(prev_state) after each POCSAG
  // TX completes, which reconfigures the ADF7021 away from POCSAG mode.
  // CMD_POCSAG_DATA only calls setMode(STATE_POCSAG) — and therefore
  // io.ifConf(STATE_POCSAG) — when the modem is in STATE_IDLE.
  // Without this, frames 2+ transmit with wrong ADF7021 configuration.
  while (MMDVM_SERIAL.available()) MMDVM_SERIAL.read();
  uint8_t idleCmd[1] = { MODE_IDLE };
  sendMMDVMCommand(CMD_SET_MODE, idleCmd, 1);
  waitForModemAck(1000);
  vTaskDelay(50 / portTICK_PERIOD_MS);

  // Update OLED display with this message's RIC and text
  setPocsagTxInfo(String(ric), message);

  // Send via MMDVM serial protocol
  sendMMDVMCommand(CMD_POCSAG_DATA, frameBytes, POCSAG_FRAME_BYTES);
  waitForModemAck(200); // no ACK on success, only NAK on failure

  // Wait for RF transmission to complete (~1.1s + guard)
  vTaskDelay(2500 / portTICK_PERIOD_MS);
}

// --- Restore modem to normal mode after POCSAG ---
static void restoreModemAfterPocsag()
{
  while (MMDVM_SERIAL.available()) MMDVM_SERIAL.read(); // flush before SET_FREQ
  sendFrequency(dmrRxFreq, dmrTxFreq, dmrRfPower);
  waitForModemAck(1000);
  vTaskDelay(50 / portTICK_PERIOD_MS);

  uint8_t config[23];
  memset(config, 0, sizeof(config));
  config[0] = 0x80;
  uint8_t modeEnables = 0x00;
  if (modeDstarEnabled) modeEnables |= 0x01;
  if (modeDmrEnabled)   modeEnables |= 0x02;
  if (modeYsfEnabled)   modeEnables |= 0x04;
  if (modeP25Enabled)   modeEnables |= 0x08;
  if (modeNxdnEnabled)  modeEnables |= 0x10;
  if (modePocsagEnabled) modeEnables |= 0x20;
  config[1] = modeEnables;
  config[2] = MMDVM_TX_DELAY;
  config[3] = MODE_IDLE;
  config[4] = 0xFF;  config[5] = 0xFF;
  config[6] = dmrColorCode;
  config[9] = 0xFF; config[10] = 0xFF; config[11] = 0xFF;
  config[12] = 0xFF; config[15] = 0xFF; config[17] = 0xFF;
  config[21] = 0xFF;
  while (MMDVM_SERIAL.available()) MMDVM_SERIAL.read(); // flush before SET_CONFIG
  sendMMDVMCommand(CMD_SET_CONFIG, config, 23);
  waitForModemAck(1000);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  addLogMessage("[POCSAG] Modem restored to idle");
}

// --- Public: send a single POCSAG message (full setup + teardown) ---
void sendPocsagMessageToModem(uint32_t ric, const String &message, uint8_t functional)
{
  pocsagTxInProgress = true;
  setModemPocsagMode();
  sendPocsagFrame(ric, message, functional);
  restoreModemAfterPocsag();
  pocsagTxInProgress = false;
}

TaskHandle_t pocsagTaskHandle = NULL;
QueueHandle_t pocsagQueue = NULL;
volatile bool pocsagTxInProgress = false;

// Shadow display array — mirrors the live queue for web UI inspection
PocsagQueueItem pocsagQueueDisplay[POCSAG_QUEUE_SIZE];
volatile uint8_t pocsagQueueDisplayCount = 0;
static portMUX_TYPE pocsagDisplayMux = portMUX_INITIALIZER_UNLOCKED;

// Check whether a RIC appears in a comma-separated list string
static bool ricInList(uint32_t ric, const String& list) {
  if (list.length() == 0) return false;
  String target = String(ric);
  int idx = 0;
  while (idx < (int)list.length()) {
    int comma = list.indexOf(',', idx);
    if (comma < 0) comma = (int)list.length();
    String token = list.substring(idx, comma);
    token.trim();
    if (token == target) return true;
    idx = comma + 1;
  }
  return false;
}

// Queue a POCSAG message for transmission (non-blocking)
bool queuePocsagMessage(uint32_t ric, const String& message, uint8_t functional)
{
  if (pocsagQueue == NULL) {
    addLogMessage("[POCSAG] Queue not initialized");
    return false;
  }

  // Whitelist / blacklist gate — checked before anything hits the queue
  // extern String pocsagBlacklist;
  // extern String pocsagWhitelist;
  if (ricInList(ric, pocsagBlacklist)) {
    addLogMessage("[POCSAG] RIC " + String(ric) + " blocked by blacklist - dropped");
    String pocsagJson = String("{\"event\":\"blacklist_drop\",\"ric\":") + ric + ",\"functional\":" + functional + ",\"message\":\"" + message + "\"}";
    publishMqtt(mqttPocsagTaskTopic.c_str(), pocsagJson);
    return false;
  }
  if (pocsagWhitelist.length() > 0 && !ricInList(ric, pocsagWhitelist)) {
    addLogMessage("[POCSAG] RIC " + String(ric) + " not in whitelist - dropped");
    String pocsagJson = String("{\"event\":\"whitelist_drop\",\"ric\":") + ric + ",\"functional\":" + functional + ",\"message\":\"" + message + "\"}";
    publishMqtt(mqttPocsagTaskTopic.c_str(), pocsagJson);
    return false;
  }

  // Forward to Telegram if this RIC is in the watch list
  checkTelegramRicForward(ric, functional, message.c_str());

  PocsagQueueItem item;
  item.ric = ric;
  item.functional = functional;
  strlcpy(item.message, message.c_str(), sizeof(item.message));

  // Enqueue to FreeRTOS queue first. Only update the shadow display array
  // after a successful enqueue — this prevents ghost entries appearing in
  // the web UI for messages that never actually made it into the queue.
  // The rare race where pocsagTask dequeues before the shadow add is handled
  // by a periodic resync in the task's idle path (see pocsagTask below).
  if (xQueueSend(pocsagQueue, &item, 0) != pdTRUE) {
    addLogMessage("[POCSAG] Queue full - message dropped");
    return false;
  }

  taskENTER_CRITICAL(&pocsagDisplayMux);
  if (pocsagQueueDisplayCount < POCSAG_QUEUE_SIZE) {
    pocsagQueueDisplay[pocsagQueueDisplayCount++] = item;
  }
  taskEXIT_CRITICAL(&pocsagDisplayMux);

  UBaseType_t waiting = uxQueueMessagesWaiting(pocsagQueue);
  addLogMessage(String("[POCSAG] Message queued (") + String(waiting) + "/" + String(POCSAG_QUEUE_SIZE) + " in queue)");
  return true;
}

void initPocsagQueue()
{
  if (pocsagQueue != NULL) return; // already created
  pocsagQueue = xQueueCreate(POCSAG_QUEUE_SIZE, sizeof(PocsagQueueItem));
  if (pocsagQueue == NULL) {
    addLogMessage("[POCSAG] ERROR: Failed to create message queue");
  } else {
    addLogMessage("[POCSAG] Message queue created (capacity: " + String(POCSAG_QUEUE_SIZE) + ")");
  }
}

void initPocsagTask()
{
  initPocsagQueue(); // ensure queue exists even if called independently

  BaseType_t result = xTaskCreatePinnedToCore(
      pocsagTask,
      "POCSAG Task",
      MMDVM_POCSAG_STACK,
      NULL,
      MMDVM_POCSAG_PRIORITY,
      &pocsagTaskHandle,
      1 // Run on core 1
  );
  if (result != pdPASS)
    log_e("[POCSAG] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());
}

/*
 * TASK: POCSAG Protocol Handler
 * Priority: Medium - Paging protocol, less time-critical than voice
 */
void pocsagTask(void *parameter)
{
  addLogMessage("[POCSAG Task] Started - waiting for network connection...");

  // Wait for any network connection (WiFi OR Ethernet)
  while (WiFi.status() != WL_CONNECTED && !ethConnected)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  addLogMessage("[POCSAG Task] Network connected - initializing POCSAG protocol");

  // Wait 5 seconds before blinking to separate from LED task
  vTaskDelay(5000 / portTICK_PERIOD_MS);

  // Blink LED 3 times to show task is running
  pinMode(LED_PIN, OUTPUT);
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    digitalWrite(LED_PIN, LOW);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  addLogMessage("[POCSAG Task] Initialized - Ready for POCSAG traffic");

  // Main loop - process queued messages
  PocsagQueueItem item;
  bool modemInPocsagMode = false;

  while (true)
  {
    // Block up to 100ms waiting for a message
    if (xQueueReceive(pocsagQueue, &item, 100 / portTICK_PERIOD_MS) == pdTRUE) {
      // Remove first entry from shadow display array (shift left)
      taskENTER_CRITICAL(&pocsagDisplayMux);
      if (pocsagQueueDisplayCount > 0) {
        for (uint8_t i = 0; i < pocsagQueueDisplayCount - 1; i++)
          pocsagQueueDisplay[i] = pocsagQueueDisplay[i + 1];
        pocsagQueueDisplayCount--;
      }
      taskEXIT_CRITICAL(&pocsagDisplayMux);
    } else {
      // Idle timeout — resync shadow count against actual queue depth.
      // Catches the rare case where an item was dequeued before its shadow
      // entry was written, leaving displayCount stuck one too high.
      UBaseType_t actualDepth = uxQueueMessagesWaiting(pocsagQueue);
      if (pocsagQueueDisplayCount != (uint8_t)actualDepth) {
        taskENTER_CRITICAL(&pocsagDisplayMux);
        if (pocsagQueueDisplayCount > (uint8_t)actualDepth)
          pocsagQueueDisplayCount = (uint8_t)actualDepth;
        taskEXIT_CRITICAL(&pocsagDisplayMux);
      }
      continue; // nothing to transmit this iteration
    }
    // Wait for modem to be idle (no DMR TX) for at least 2 seconds
      if (dmrTxActive) {
        addLogMessage("[POCSAG Task] Waiting for DMR TX to finish...");
        // Restore modem first if we somehow ended up here mid-POCSAG
        if (modemInPocsagMode) {
          restoreModemAfterPocsag();
          modemInPocsagMode = false;
          pocsagTxInProgress = false;
        }
        while (true) {
          while (dmrTxActive) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
          }
          bool stayedIdle = true;
          for (int i = 0; i < 20; i++) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            if (dmrTxActive) {
              stayedIdle = false;
              addLogMessage("[POCSAG Task] DMR TX restarted - waiting again...");
              break;
            }
          }
          if (stayedIdle) break;
        }
        addLogMessage("[POCSAG Task] Modem idle for 2s - proceeding");
      }

      UBaseType_t remaining = uxQueueMessagesWaiting(pocsagQueue);
      addLogMessage(String("[POCSAG Task] Processing message (") + String(remaining) + " remaining in queue)");

      // Set up POCSAG mode once for a batch of messages.
      // Set pocsagTxInProgress BEFORE setup so the DMR task stops reading
      // MMDVM_SERIAL — otherwise it consumes the ACK bytes we're waiting for.
      if (!modemInPocsagMode) {
        // If modemTask is using the modem for CW ID (or similar), wait for it
        // to finish before we start sending POCSAG commands. Without this guard
        // a POCSAG message arriving during CW ID would immediately cut the CW off.
        if (pocsagTxInProgress) {
          addLogMessage("[POCSAG Task] Waiting for modem to become free (CW ID in progress)...");
          while (pocsagTxInProgress) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
          }
        }
        pocsagTxInProgress = true;
        modemInPocsagMode = true;
        setModemPocsagMode();
      }

      sendPocsagFrame(item.ric, String(item.message), item.functional);

      // Restore only when the queue is now empty so we don't pay the
      // re-setup cost for every consecutive message.
      if (uxQueueMessagesWaiting(pocsagQueue) == 0) {
        restoreModemAfterPocsag();
        modemInPocsagMode = false;
        pocsagTxInProgress = false;
      }

    // TODO: Handle DAPNET network messages
  }
}
