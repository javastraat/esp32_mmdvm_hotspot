/*
 * WiFi System Module Implementation
 */

#include "system/system_wifi.h"
#include "system/system_logger.h"
#include "include/config.h"
#include "system/service_mqtt.h"
#include "esp_netif.h"
#include "lwip/dns.h"

#include <ESPmDNS.h>

// External: Ethernet connection status
extern volatile bool ethConnected;
extern bool mdnsEnabled;
extern String mdnsHostname;
extern bool dnsFallbackEnabled;
extern String dnsFallbackIp;
extern String mqttWifiTaskTopic;

// External: WiFi slot arrays
#define WIFI_SLOT_COUNT 6
extern String wifiSlotLabel[WIFI_SLOT_COUNT];
extern String wifiSlotSsid[WIFI_SLOT_COUNT];
extern String wifiSlotPass[WIFI_SLOT_COUNT];

TaskHandle_t wifiTaskHandle = NULL;
bool softAPActive = false; // Track if Soft AP is running

/*
 * Add fallback DNS server (called after DHCP completes)
 */
void addFallbackDNS()
{
  if (!dnsFallbackEnabled)
    return;
  ip_addr_t dns_fallback;
  ipaddr_aton(dnsFallbackIp.c_str(), &dns_fallback);
  dns_setserver(2, &dns_fallback); // Index 2 = tertiary/fallback DNS
  addLogMessage("[Network] Fallback DNS added: " + dnsFallbackIp);
  publishMqtt(mqttWifiTaskTopic.c_str(),
              "{\"event\":\"dns_fallback_added\",\"ip\":\"" + dnsFallbackIp + "\"}");
}

void initWiFiTask()
{
  BaseType_t result = xTaskCreatePinnedToCore(
      wifiTask,
      "WiFi Task",
      WIFI_TASK_STACK,
      NULL,
      WIFI_TASK_PRIORITY,
      &wifiTaskHandle,
      0 // Run on core 0 (network stack)
  );
  if (result != pdPASS)
    log_e("[WiFi] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());
}

/*
 * Start Soft AP fallback mode
 */
void startSoftAP()
{
  if (softAPActive)
    return;

  extern String wifiApSsid;
  extern String wifiApPassword;
  extern uint8_t wifiApChannel;

  addLogMessage("[WiFi Task] Starting Soft AP fallback...");
  publishMqtt(mqttWifiTaskTopic.c_str(), "{\"event\":\"softap_starting\"}");
  WiFi.mode(WIFI_AP_STA); // Allow both AP and STA
  WiFi.softAP(wifiApSsid.c_str(), wifiApPassword.c_str(), wifiApChannel);

  softAPActive = true;
  String apIp = WiFi.softAPIP().toString();
  addLogMessage("[WiFi Task] Soft AP started: " + wifiApSsid);
  addLogMessage("[WiFi Task] AP IP: " + apIp);
  publishMqtt(mqttWifiTaskTopic.c_str(),
              "{\"event\":\"softap_started\",\"ssid\":\"" + wifiApSsid + "\",\"ip\":\"" + apIp + "\"}");

  // Start mDNS on AP
  if (mdnsEnabled)
  {
    if (MDNS.begin(mdnsHostname.c_str()))
    {
      addLogMessage("[WiFi Task] mDNS started on AP: " + mdnsHostname + ".local");
      publishMqtt(mqttWifiTaskTopic.c_str(),
                  "{\"event\":\"mdns_started\",\"hostname\":\"" + mdnsHostname + ".local\",\"interface\":\"ap\"}");
      MDNS.addService("http", "tcp", WEB_SERVER_PORT);
    }
  }
}

/*
 * Stop Soft AP when regular connection is available
 */
void stopSoftAP()
{
  if (!softAPActive)
    return;

  addLogMessage("[WiFi Task] Stopping Soft AP (network available)");
  publishMqtt(mqttWifiTaskTopic.c_str(), "{\"event\":\"softap_stopped\"}");
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  softAPActive = false;
}

/*
 * Try connecting to a single WiFi slot. Returns true if connected.
 * Checks for Ethernet connection and respects timeout.
 */
bool tryConnectSlot(int slot)
{
  if (wifiSlotSsid[slot].isEmpty())
    return false;

  extern uint8_t wifiMaxRetries;
  String label = wifiSlotLabel[slot].isEmpty() ? String(slot) : wifiSlotLabel[slot];
  addLogMessage("[WiFi Task] Trying slot " + String(slot) + " (" + label + "): " + wifiSlotSsid[slot]);
  publishMqtt(mqttWifiTaskTopic.c_str(),
              "{\"event\":\"connecting\",\"slot\":" + String(slot) +
              ",\"label\":\"" + label + "\",\"ssid\":\"" + wifiSlotSsid[slot] + "\"}");

  WiFi.disconnect();
  WiFi.begin(wifiSlotSsid[slot].c_str(), wifiSlotPass[slot].c_str());

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    if (ethConnected)
    {
      addLogMessage("[WiFi Task] Ethernet connected - WiFi optional");
      publishMqtt(mqttWifiTaskTopic.c_str(), "{\"event\":\"eth_connected_wifi_optional\"}");
      return false;
    }
    if (millis() - startTime > WIFI_CONNECT_TIMEOUT)
    {
      addLogMessage("[WiFi Task] Slot " + String(slot) + " (" + label + ") timeout");
      publishMqtt(mqttWifiTaskTopic.c_str(),
                  "{\"event\":\"connect_timeout\",\"slot\":" + String(slot) +
                  ",\"label\":\"" + label + "\"}");
      return false;
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  return true;
}

/*
 * Try all WiFi slots in order (0=Default, 1-5). Returns true if any connected.
 */
bool tryAllSlots()
{
  for (int i = 0; i < WIFI_SLOT_COUNT; i++)
  {
    if (ethConnected)
      return false;
    if (tryConnectSlot(i))
      return true;
  }
  return false;
}

/*
 * Handle post-connection setup (DNS, mDNS)
 */
void onWiFiConnected()
{
  String ip = WiFi.localIP().toString();
  addLogMessage("[WiFi Task] Connected! IP " + ip);
  publishMqtt(mqttWifiTaskTopic.c_str(),
              "{\"event\":\"connected\",\"ip\":\"" + ip + "\"}");

  addFallbackDNS();

  if (mdnsEnabled)
  {
    if (MDNS.begin(mdnsHostname.c_str()))
    {
      addLogMessage("[WiFi Task] mDNS started: " + mdnsHostname + ".local");
      publishMqtt(mqttWifiTaskTopic.c_str(),
                  "{\"event\":\"mdns_started\",\"hostname\":\"" + mdnsHostname + ".local\",\"interface\":\"sta\"}");
      MDNS.addService("http", "tcp", WEB_SERVER_PORT);
    }
    else
    {
      addLogMessage("[WiFi Task] mDNS failed to start");
      publishMqtt(mqttWifiTaskTopic.c_str(), "{\"event\":\"mdns_failed\"}");
    }
  }
}

/*
 * TASK: WiFi Connection Monitor
 * Priority: High - Network connectivity is important
 *
 * Tries all WiFi slots (Default, 1-5) in order.
 * Falls back to Soft AP if no slot connects after max retries.
 */
void wifiTask(void *parameter)
{
  addLogMessage("[WiFi Task] Started");
  publishMqtt(mqttWifiTaskTopic.c_str(), "{\"event\":\"started\"}");

  extern uint8_t wifiMaxRetries;
  int retryCount = 0;

  WiFi.mode(WIFI_STA);

  // Initial connection - try all slots
  if (tryAllSlots())
  {
    onWiFiConnected();
    retryCount = 0;
  }
  else if (!ethConnected)
  {
    retryCount++;
    addLogMessage("[WiFi Task] All slots failed (attempt " + String(retryCount) + "/" + String(wifiMaxRetries) + ")");
    publishMqtt(mqttWifiTaskTopic.c_str(),
                "{\"event\":\"all_slots_failed\",\"attempt\":" + String(retryCount) +
                ",\"max\":" + String(wifiMaxRetries) + "}");
  }

  // Monitor connection
  while (true)
  {
    // Check if we should start Soft AP (no WiFi, no Ethernet, max retries reached)
    if (WiFi.status() != WL_CONNECTED && !ethConnected && retryCount >= wifiMaxRetries)
    {
      startSoftAP();
    }

    // Stop Soft AP if we have network connectivity
    if (softAPActive && (WiFi.status() == WL_CONNECTED || ethConnected))
    {
      stopSoftAP();
      retryCount = 0;
    }

    // Reconnect if WiFi lost and no AP/Ethernet
    if (WiFi.status() != WL_CONNECTED && !softAPActive && !ethConnected)
    {
      addLogMessage("[WiFi Task] Connection lost! Trying all slots...");
      publishMqtt(mqttWifiTaskTopic.c_str(), "{\"event\":\"connection_lost\"}");

      if (tryAllSlots())
      {
        onWiFiConnected();
        retryCount = 0;
      }
      else if (!ethConnected)
      {
        retryCount++;
        addLogMessage("[WiFi Task] All slots failed (attempt " + String(retryCount) + "/" + String(wifiMaxRetries) + ")");
        publishMqtt(mqttWifiTaskTopic.c_str(),
                    "{\"event\":\"all_slots_failed\",\"attempt\":" + String(retryCount) +
                    ",\"max\":" + String(wifiMaxRetries) + "}");
      }
    }

    // Reset retry count if Ethernet is connected
    if (ethConnected && retryCount > 0)
    {
      retryCount = 0;
    }

    // Check every 5 seconds
    vTaskDelay(WIFI_CHECK_INTERVAL / portTICK_PERIOD_MS);
  }
}
