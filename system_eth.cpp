/*
 * Ethernet System Module Implementation
 */

// ...existing code...
#include "system/system_eth.h"
#include "system/system_logger.h"
#include "include/config.h"
#include "system/service_mqtt.h"
#include "lwip/dns.h"

#include <ESPmDNS.h>

// External runtime settings
extern bool mdnsEnabled;
extern String mdnsHostname;
extern bool dnsFallbackEnabled;
extern String dnsFallbackIp;
extern bool ethDebug;
extern String mqttEthTaskTopic;

TaskHandle_t ethTaskHandle = NULL;
volatile bool ethConnected = false;

/*
 * Add fallback DNS server (called after DHCP completes)
 */
void addEthFallbackDNS()
{
  if (!dnsFallbackEnabled)
    return;
  ip_addr_t dns_fallback;
  ipaddr_aton(dnsFallbackIp.c_str(), &dns_fallback);
  dns_setserver(2, &dns_fallback); // Index 2 = tertiary/fallback DNS
  addLogMessage("[Ethernet Task] Fallback DNS added: " + dnsFallbackIp);
  publishMqtt(mqttEthTaskTopic.c_str(),
              "{\"event\":\"dns_fallback_added\",\"ip\":\"" + dnsFallbackIp + "\"}");
}

void initEthTask()
{
  BaseType_t result = xTaskCreatePinnedToCore(
      ethTask,
      "Ethernet Task",
      ETH_TASK_STACK,
      NULL,
      ETH_TASK_PRIORITY,
      &ethTaskHandle,
      0 // Run on core 0 (network management not time-critical)
  );
  if (result != pdPASS)
    log_e("[Ethernet] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());
}

/*
 * TASK: Ethernet Connection Monitor
 * Priority: High - Network connectivity is important
 *
 * This task initializes and monitors Ethernet connection
 * Uses SPI-based W5500 Ethernet module
 */
void ethTask(void *parameter)
{
  addLogMessage("[Ethernet Task] Started");
  publishMqtt(mqttEthTaskTopic.c_str(), "{\"event\":\"started\"}");

  // Setup SPI for Ethernet
  SPI.begin(ETH_SCLK_PIN, ETH_MISO_PIN, ETH_MOSI_PIN, ETH_CS_PIN);

  // Register event handler
  Network.onEvent(onEthEvent);

  // Initialize Ethernet with SPI
  addLogMessage("[Ethernet Task] Initializing W5500...");
  publishMqtt(mqttEthTaskTopic.c_str(), "{\"event\":\"initializing\"}");

#if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  // For older Arduino ESP32 versions
  ETH.begin(ETH_ADDR, ETH_CS_PIN, ETH_INT_PIN, ETH_RST_PIN, SPI);
#else
  // For Arduino ESP32 v3.0.0+
  ETH.begin(
      ETH_PHY_W5500, // PHY type
      ETH_ADDR,      // PHY address
      ETH_CS_PIN,    // CS pin
      ETH_INT_PIN,   // INT pin
      ETH_RST_PIN,   // RST pin
      SPI            // SPI instance
  );
#endif

  addLogMessage("[Ethernet Task] Waiting for connection...");
  publishMqtt(mqttEthTaskTopic.c_str(), "{\"event\":\"waiting\"}");

  // Wait for initial connection with timeout
  unsigned long startTime = millis();
  bool timedOut = false;

  while (!ethConnected)
  {
    if (millis() - startTime > ETH_CONNECT_TIMEOUT)
    {
      addLogMessage("[Ethernet Task] Connection timeout - continuing without Ethernet");
      addLogMessage("[Ethernet Task] Check cable and network settings");
      publishMqtt(mqttEthTaskTopic.c_str(), "{\"event\":\"connect_timeout\"}");
      timedOut = true;
      break;
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }

  if (ethConnected)
  {
    addLogMessage("[Ethernet Task] Connected! IP: " + ETH.localIP().toString() + " MAC: " + ETH.macAddress());
    publishMqtt(mqttEthTaskTopic.c_str(),
                "{\"event\":\"connected\",\"ip\":\"" + ETH.localIP().toString() +
                "\",\"mac\":\"" + ETH.macAddress() + "\"}");
  }

  // Monitor connection
  bool wasConnected = ethConnected;
  while (true)
  {
    if (ethConnected && !wasConnected)
    {
      // Ethernet came online (possibly after initial timeout)
      addLogMessage("[Ethernet Task] Connected! IP: " + ETH.localIP().toString() + " MAC: " + ETH.macAddress());
      publishMqtt(mqttEthTaskTopic.c_str(),
                  "{\"event\":\"connected\",\"ip\":\"" + ETH.localIP().toString() +
                  "\",\"mac\":\"" + ETH.macAddress() + "\"}");
    }
    else if (!ethConnected && wasConnected)
    {
      addLogMessage("[Ethernet Task] Connection lost!");
      publishMqtt(mqttEthTaskTopic.c_str(), "{\"event\":\"connection_lost\"}");
    }
    wasConnected = ethConnected;

    // Check every 5 seconds
    vTaskDelay(ETH_CHECK_INTERVAL / portTICK_PERIOD_MS);
  }
}

/*
 * Ethernet Event Handler
 * Called when Ethernet events occur (connected, disconnected, got IP, etc.)
 */
void onEthEvent(arduino_event_id_t event)
{
  switch (event)
  {
  case ARDUINO_EVENT_ETH_START:
    if (ethDebug)
    {
      addLogMessage("[Ethernet Task] ETH Started");
      publishMqtt(mqttEthTaskTopic.c_str(), "{\"event\":\"eth_started\"}");
    }
    ETH.setHostname("esp32-rtos");
    break;

  case ARDUINO_EVENT_ETH_CONNECTED:
    addLogMessage("[Ethernet Task] Link up");
    publishMqtt(mqttEthTaskTopic.c_str(), "{\"event\":\"link_up\"}");
    break;

  case ARDUINO_EVENT_ETH_GOT_IP:
    addLogMessage("[Ethernet Task] Got IP: " + ETH.localIP().toString() +
                  " Speed: " + String(ETH.linkSpeed()) + " Mbps " +
                  (ETH.fullDuplex() ? "Full" : "Half") + "-duplex");
    publishMqtt(mqttEthTaskTopic.c_str(),
                "{\"event\":\"got_ip\",\"ip\":\"" + ETH.localIP().toString() +
                "\",\"speed_mbps\":" + String(ETH.linkSpeed()) +
                ",\"duplex\":\"" + String(ETH.fullDuplex() ? "Full" : "Half") + "\"}");
    ethConnected = true;

    // Add fallback DNS server
    addEthFallbackDNS();

    // Start mDNS service if not already started
    if (mdnsEnabled)
    {
      if (!MDNS.begin(mdnsHostname.c_str()))
      {
        // If begin fails, try to restart it
        MDNS.end();
        if (MDNS.begin(mdnsHostname.c_str()))
        {
          addLogMessage("[Ethernet Task] mDNS started: " + mdnsHostname + ".local");
          publishMqtt(mqttEthTaskTopic.c_str(),
                      "{\"event\":\"mdns_started\",\"hostname\":\"" + mdnsHostname + ".local\",\"interface\":\"eth\"}");
          MDNS.addService("http", "tcp", WEB_SERVER_PORT);
        }
        else
        {
          addLogMessage("[Ethernet Task] mDNS failed to start");
          publishMqtt(mqttEthTaskTopic.c_str(), "{\"event\":\"mdns_failed\"}");
        }
      }
      else
      {
        addLogMessage("[Ethernet Task] mDNS started: " + mdnsHostname + ".local");
        publishMqtt(mqttEthTaskTopic.c_str(),
                    "{\"event\":\"mdns_started\",\"hostname\":\"" + mdnsHostname + ".local\",\"interface\":\"eth\"}");
        MDNS.addService("http", "tcp", WEB_SERVER_PORT);
      }
    }
    break;

  case ARDUINO_EVENT_ETH_DISCONNECTED:
    addLogMessage("[Ethernet Task] Disconnected");
    publishMqtt(mqttEthTaskTopic.c_str(), "{\"event\":\"disconnected\"}");
    ethConnected = false;
    break;

  case ARDUINO_EVENT_ETH_STOP:
    addLogMessage("[Ethernet Task] Stopped");
    publishMqtt(mqttEthTaskTopic.c_str(), "{\"event\":\"stopped\"}");
    ethConnected = false;
    break;

  default:
    break;
  }
}
