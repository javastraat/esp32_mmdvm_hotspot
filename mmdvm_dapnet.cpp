/*
 * DAPNET Network Client Task — TCP transmitter protocol
 *
 * Protocol (TCP port 43434):
 *   Client → "[<software> v<ver> <callsign> <authkey>]\n"
 *   Server → "2:<hex>"  (5×)   Client → "2:<hex>:0000\n+\n"
 *   Server → "3:+<hex>"         Client → "+\n"
 *   Server → "4:<hex>"          Client → "+\n"
 *   Server → "#<ctr_hex> 6:1:<RIC_hex>:<func>:<message>"
 *   Client → "#<ctr_hex+1> +\n"
 *   (login failure: "7 Invalid credentials")
 *
 * Counter is 8-bit hex, wraps 0xFF → 0x00.
 * RIC field in page lines is hexadecimal.
 */

#include "include/config.h"
#include "system/system_logger.h"
#include "mmdvm/mmdvm_dapnet.h"
#include <time.h>
#include "mmdvm/mmdvm_pocsag.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include "system/service_mqtt.h"
#include "system/system_eth.h"
#include "system/system_espnow.h"

extern String firmwareVersion;
extern String dapnetServer;
extern uint16_t dapnetPort;
extern String dapnetNodeCs;
extern String dapnetAuthKey;
extern String userCallsign;
extern String mqttDapnetTaskTopic;
extern bool   pocsagServerEspNow;

TaskHandle_t dapnetTaskHandle = nullptr;
volatile bool dapnetLoggedIn = false;

// ---------------------------------------------------------------------------
// Last 15 received DAPNET message history (newest overwrites oldest)
// ---------------------------------------------------------------------------
#define DAPNET_MSG_HISTORY 15
struct DapnetMsgEntry {
    uint32_t ric;
    uint8_t  func;
    String   msg;
    String   receivedTime; // "HH:MM:SS"
};
static DapnetMsgEntry dapnetMsgHistory[DAPNET_MSG_HISTORY];
static int dapnetMsgHistoryCount = 0;
static int dapnetMsgHistoryHead  = 0; // next write slot

static void addToMessageHistory(uint32_t ric, uint8_t func, const String& msg) {
    char timeBuf[9];
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    dapnetMsgHistory[dapnetMsgHistoryHead] = {ric, func, msg, String(timeBuf)};
    dapnetMsgHistoryHead = (dapnetMsgHistoryHead + 1) % DAPNET_MSG_HISTORY;
    if (dapnetMsgHistoryCount < DAPNET_MSG_HISTORY) dapnetMsgHistoryCount++;
}

String getDapnetMessageHistoryJson() {
    String json = "{\"count\":" + String(dapnetMsgHistoryCount) + ",\"items\":[";
    // Walk from oldest → newest
    int start = (dapnetMsgHistoryCount < DAPNET_MSG_HISTORY) ? 0 : dapnetMsgHistoryHead;
    for (int i = 0; i < dapnetMsgHistoryCount; i++) {
        int idx = (start + i) % DAPNET_MSG_HISTORY;
        if (i > 0) json += ",";
        String escapedMsg = dapnetMsgHistory[idx].msg;
        escapedMsg.replace("\\", "\\\\");
        escapedMsg.replace("\"", "\\\"");
        json += "{\"ric\":" + String(dapnetMsgHistory[idx].ric) +
                ",\"func\":" + String(dapnetMsgHistory[idx].func) +
                ",\"msg\":\"" + escapedMsg + "\"" +
                ",\"time\":\"" + dapnetMsgHistory[idx].receivedTime + "\"}";
    }
    json += "]}";
    return json;
}

// ---------------------------------------------------------------------------
// Read one line (up to '\n') from TCP with a timeout.
// Returns "" on timeout or disconnect.
// ---------------------------------------------------------------------------
static String tcpReadLine(WiFiClient& client, unsigned long timeoutMs)
{
    unsigned long start = millis();
    while (client.connected() && (millis() - start) < timeoutMs) {
        if (client.available()) {
            String line = client.readStringUntil('\n');
            line.trim();
            return line;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    return "";
}

// ---------------------------------------------------------------------------
// Parse and queue a DAPNET page line.
//
// Format: "#<ctr_hex> 6:1:<RIC_hex>:<func>:<message>"
// Returns the counter byte so the caller can ACK it.
// Returns -1 if the line is not a recognisable page.
// ---------------------------------------------------------------------------
static int handlePageLine(const String& line)
{
    if (!line.startsWith("#")) return -1;

    int spacePos = line.indexOf(' ');
    if (spacePos < 2) return -1;
    String ctrStr = line.substring(1, spacePos);
    int counter = (int)strtol(ctrStr.c_str(), nullptr, 16);

    String rest = line.substring(spacePos + 1);
    int colon1 = rest.indexOf(':');
    if (colon1 < 0) return counter;
    int msgType = rest.substring(0, colon1).toInt();

    if (msgType != 6) {
        addLogMessage("[DAPNET Task] Received type-" + String(msgType) + " (not a page), ctr=" + ctrStr);
        publishMqtt(mqttDapnetTaskTopic.c_str(),
                    "{\"event\":\"timesync\",\"type\":" + String(msgType) + "}");
        return counter;
    }

    int colon2 = rest.indexOf(':', colon1 + 1);
    if (colon2 < 0) return counter;

    int colon3 = rest.indexOf(':', colon2 + 1);
    if (colon3 < 0) return counter;
    String ricHex = rest.substring(colon2 + 1, colon3);
    uint32_t ric = (uint32_t)strtoul(ricHex.c_str(), nullptr, 16);

    int colon4 = rest.indexOf(':', colon3 + 1);
    if (colon4 < 0) return counter;
    uint8_t functional = (uint8_t)rest.substring(colon3 + 1, colon4).toInt();

    String message = rest.substring(colon4 + 1);
    message.trim();

    addLogMessage("[DAPNET Task] Page ctr=" + ctrStr +
                  " RIC=" + String(ric) +
                  " func=" + String(functional) +
                  " msg=" + message);

    // Escape quotes in message before embedding in JSON
    String escapedMsg = message;
    escapedMsg.replace("\\", "\\\\");
    escapedMsg.replace("\"", "\\\"");
    publishMqtt(mqttDapnetTaskTopic.c_str(),
                "{\"event\":\"page\""
                ",\"ric\":" + String(ric) +
                ",\"func\":" + String(functional) +
                ",\"msg\":\"" + escapedMsg + "\"}");

    queuePocsagMessage(ric, message, functional);
    addToMessageHistory(ric, functional, message);
    return counter;
}

// ---------------------------------------------------------------------------
// DAPNET task
// ---------------------------------------------------------------------------
void dapnetTask(void* parameter)
{
    addLogMessage("[DAPNET Task] Task started");
    publishMqtt(mqttDapnetTaskTopic.c_str(), "{\"event\":\"started\"}");

#if ESPNOW_SENDER
    // ESP-NOW relay mode: skip DAPNET TCP entirely, drain ESP-NOW POCSAG queue
    if (pocsagServerEspNow) {
        addLogMessage("[DAPNET Task] ESP-NOW relay mode — skipping DAPNET server, receiving via ESP-NOW");
        publishMqtt(mqttDapnetTaskTopic.c_str(), "{\"event\":\"espnow_relay_ready\"}");
        dapnetLoggedIn = true;
        for (;;) {
            if (espnowPocsagQueue) {
                EspNowPocsagPacket pkt;
                while (xQueueReceive(espnowPocsagQueue, &pkt, 0) == pdTRUE) {
                    if (pkt.type == ESPNOW_TYPE_POCSAG) {
                        pkt.message[POCSAG_MSG_MAX_LEN] = '\0';
                        String msg = String(pkt.message);
                        addLogMessage("[DAPNET Task] ESP-NOW page RIC=" + String(pkt.ric) +
                                      " func=" + String(pkt.functional) + " msg=" + msg);
                        queuePocsagMessage(pkt.ric, msg, pkt.functional);
                        addToMessageHistory(pkt.ric, pkt.functional, msg);
                    }
                }
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
#endif

    for (;;) {
        while (WiFi.status() != WL_CONNECTED && !ethConnected) {
            vTaskDelay(pdMS_TO_TICKS(5000));
        }

        String cs = (dapnetNodeCs.length() > 0) ? dapnetNodeCs : userCallsign;
        cs.toUpperCase();

        addLogMessage("[DAPNET Task] Connecting to " + dapnetServer +
                      ":" + String(dapnetPort) + " as " + cs);
        publishMqtt(mqttDapnetTaskTopic.c_str(),
                    "{\"event\":\"connecting\""
                    ",\"server\":\"" + dapnetServer + "\"" +
                    ",\"port\":" + String(dapnetPort) +
                    ",\"callsign\":\"" + cs + "\"}");

        // ── TCP connect ─────────────────────────────────────────────────────
        IPAddress serverIP;
        if (!WiFi.hostByName(dapnetServer.c_str(), serverIP)) {
            addLogMessage("[DAPNET Task] DNS resolve failed for " + dapnetServer + ", retry in 30s");
            publishMqtt(mqttDapnetTaskTopic.c_str(),
                        "{\"event\":\"dns_failed\",\"server\":\"" + dapnetServer + "\"}");
            vTaskDelay(pdMS_TO_TICKS(30000));
            continue;
        }
        addLogMessage("[DAPNET Task] Resolved " + dapnetServer + " -> " + serverIP.toString());
        publishMqtt(mqttDapnetTaskTopic.c_str(),
                    "{\"event\":\"resolved\""
                    ",\"server\":\"" + dapnetServer + "\"" +
                    ",\"ip\":\"" + serverIP.toString() + "\"}");

        WiFiClient tcp;
        addLogMessage("[DAPNET Task] TCP connect start (15s timeout) to " + serverIP.toString());
        unsigned long t0 = millis();
        bool tcpOk = tcp.connect(serverIP, dapnetPort, 15000);
        unsigned long elapsed = millis() - t0;

        addLogMessage("[DAPNET Task] TCP connect result=" + String(tcpOk) +
                      " elapsed=" + String(elapsed) + "ms");
        publishMqtt(mqttDapnetTaskTopic.c_str(),
                    "{\"event\":\"connect_result\""
                    ",\"success\":" + String(tcpOk ? "true" : "false") +
                    ",\"elapsed_ms\":" + String(elapsed) + "}");

        if (!tcpOk) {
            addLogMessage("[DAPNET Task] TCP connect failed, retry in 30s");
            publishMqtt(mqttDapnetTaskTopic.c_str(), "{\"event\":\"connect_failed\"}");
            vTaskDelay(pdMS_TO_TICKS(30000));
            continue;
        }
        addLogMessage("[DAPNET Task] TCP connected");
        publishMqtt(mqttDapnetTaskTopic.c_str(), "{\"event\":\"connected\"}");

        // ── Send login (auth key not published to MQTT) ─────────────────────
        String login = "[MMDVM v" + firmwareVersion + " " + cs + " " + dapnetAuthKey + "]\r\n";
        tcp.print(login);
        addLogMessage("[DAPNET Task] Sent login for " + cs);

        // ── Handshake ───────────────────────────────────────────────────────
        bool loginOk = false;
        for (int step = 0; step < 20 && tcp.connected(); step++) {
            String line = tcpReadLine(tcp, 5000);
            if (line.length() == 0) {
                addLogMessage("[DAPNET Task] Handshake timeout at step " + String(step));
                publishMqtt(mqttDapnetTaskTopic.c_str(),
                            "{\"event\":\"handshake_timeout\",\"step\":" + String(step) + "}");
                break;
            }
            addLogMessage("[DAPNET Task] Handshake rx: " + line);

            if (line.startsWith("7")) {
                addLogMessage("[DAPNET Task] Login rejected: " + line);
                publishMqtt(mqttDapnetTaskTopic.c_str(),
                            "{\"event\":\"login_rejected\",\"reason\":\"" + line + "\"}");
                break;
            }
            if (line.startsWith("2:")) {
                tcp.print(line + ":0000\r\n");
                tcp.print("+\r\n");
                addLogMessage("[DAPNET Task] Time challenge replied");
                continue;
            }
            if (line.startsWith("3:")) {
                tcp.print("+\r\n");
                continue;
            }
            if (line.startsWith("4:")) {
                tcp.print("+\r\n");
                String slots = line.substring(2); // strip "4:"
                addLogMessage("[DAPNET Task] Handshake complete, time slots: " + line);
                publishMqtt(mqttDapnetTaskTopic.c_str(),
                            "{\"event\":\"handshake_ok\",\"slots\":\"" + slots + "\"}");
                loginOk = true;
                break;
            }
        }

        if (!loginOk) {
            addLogMessage("[DAPNET Task] Login failed, retry in 60s");
            publishMqtt(mqttDapnetTaskTopic.c_str(), "{\"event\":\"login_failed\"}");
            tcp.stop();
            vTaskDelay(pdMS_TO_TICKS(60000));
            continue;
        }

        dapnetLoggedIn = true;
        addLogMessage("[DAPNET Task] Logged in — listening for pages");
        publishMqtt(mqttDapnetTaskTopic.c_str(), "{\"event\":\"logged_in\"}");

        // ── Page receive loop ───────────────────────────────────────────────
        while (tcp.connected()) {
            String line = tcpReadLine(tcp, 30000);
            if (line.length() == 0) continue;

            addLogMessage("[DAPNET Task] RX: " + line);

            // Time-sync keepalives sent periodically by server — ACK them or
            // the server will drop the connection after ~60 minutes
            if (line.startsWith("2:")) { tcp.print(line + ":0000\r\n"); tcp.print("+\r\n"); continue; }
            if (line.startsWith("3:") || line.startsWith("4:") || line.startsWith("5:")) { tcp.print("+\r\n"); continue; }

            if (!line.startsWith("#")) continue;

            int counter = handlePageLine(line);
            if (counter < 0) continue;

            uint8_t nextCtr = (uint8_t)(counter + 1);
            char ack[16];
            snprintf(ack, sizeof(ack), "#%02X +\r\n", nextCtr);
            tcp.print(ack);
        }

        dapnetLoggedIn = false;
        addLogMessage("[DAPNET Task] TCP disconnected, retry in 30s");
        publishMqtt(mqttDapnetTaskTopic.c_str(), "{\"event\":\"disconnected\"}");
        tcp.stop();
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

// ---------------------------------------------------------------------------
// Start the DAPNET task (call only if dapnetEnabled)
// ---------------------------------------------------------------------------
void initDapnetTask()
{
    BaseType_t result = xTaskCreatePinnedToCore(
        dapnetTask,
        "DAPNET Task",
        MMDVM_DAPNET_STACK,
        NULL,
        MMDVM_DAPNET_PRIORITY,
        &dapnetTaskHandle,
        0 // Pure TCP I/O — should be Core 0 (see M-10)
    );
    if (result != pdPASS)
        log_e("[DAPNET] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());
}
