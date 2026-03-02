/*
 * service_wireguard.cpp - WireGuard VPN Client Implementation
 */

#include "system/service_wireguard.h"
#include "system/system_logger.h"
#include "system/service_mqtt.h"
#include "system/system_eth.h"
#include "include/config.h"
#include <WiFi.h>
#include <time.h>

// Extern globals from main .ino
extern bool wgEnabled;
extern String wgLocalIp;
extern String wgPrivateKey;
extern String wgPublicKey;
extern String wgEndpoint;
extern uint16_t wgEndpointPort;
extern String wgDns;
extern String wgAllowedIps;
extern String mqttWgTaskTopic;

// WireGuard status
bool wireguardConnected = false;
TaskHandle_t wireguardTaskHandle = NULL;

// WireGuard instance
static WireGuardVPN wg;

// Reconnect interval (30 seconds)
static const unsigned long WG_RECONNECT_INTERVAL = 30000;

/*
 * Initialize WireGuard task
 */
void initWireguardTask()
{
  BaseType_t result = xTaskCreatePinnedToCore(
      wireguardTask,      // Task function
      "WireGuard Task",   // Task name
      WG_TASK_STACK,      // Stack size
      NULL,               // Parameters
      WG_TASK_PRIORITY,   // Priority
      &wireguardTaskHandle, // Task handle
      0                   // Core 0 (network)
  );
  if (result != pdPASS)
    log_e("[WireGuard] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());

  addLogMessage("[WireGuard Task] Started");
  publishMqtt(mqttWgTaskTopic.c_str(), "{\"event\":\"started\"}");
}

/*
 * WireGuard VPN task
 */
void wireguardTask(void *parameter)
{
  addLogMessage("[WireGuard Task] Waiting for network connection...");
  publishMqtt(mqttWgTaskTopic.c_str(), "{\"event\":\"waiting_for_network\"}");

  // Wait for network (WiFi or Ethernet)
  while (WiFi.status() != WL_CONNECTED && !ethConnected)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  addLogMessage("[WireGuard Task] Network available, waiting for valid time (NTP)...");
  publishMqtt(mqttWgTaskTopic.c_str(), "{\"event\":\"network_ready\"}");

  // WireGuard requires valid system time for the handshake
  int timeWaitCount = 0;
  while (time(nullptr) < 1000000)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    timeWaitCount++;
    if (timeWaitCount >= 60)
    {
      addLogMessage("[WireGuard Task] WARNING: NTP time not available after 60s - WireGuard requires valid time!");
      publishMqtt(mqttWgTaskTopic.c_str(), "{\"event\":\"ntp_timeout_warning\"}");
      timeWaitCount = 0;
    }
  }

  addLogMessage("[WireGuard Task] Time synced, connecting to WireGuard...");
  publishMqtt(mqttWgTaskTopic.c_str(), "{\"event\":\"time_synced\"}");

  // Validate configuration
  if (wgPrivateKey.isEmpty() || wgPublicKey.isEmpty() || wgEndpoint.isEmpty() || wgLocalIp.isEmpty())
  {
    addLogMessage("[WireGuard Task] ERROR: Missing configuration (private key, public key, endpoint, or local IP)");
    publishMqtt(mqttWgTaskTopic.c_str(), "{\"event\":\"config_error\",\"reason\":\"missing_config\"}");
    vTaskSuspend(NULL);
    return;
  }

  // Attempt initial connection
  IPAddress localIp;
  if (!localIp.fromString(wgLocalIp))
  {
    addLogMessage("[WireGuard Task] ERROR: Invalid local IP address: " + wgLocalIp);
    publishMqtt(mqttWgTaskTopic.c_str(),
                "{\"event\":\"config_error\",\"reason\":\"invalid_local_ip\",\"ip\":\"" + wgLocalIp + "\"}");
    vTaskSuspend(NULL);
    return;
  }

  addLogMessage("[WireGuard Task] Connecting to " + wgEndpoint + ":" + String(wgEndpointPort));
  publishMqtt(mqttWgTaskTopic.c_str(),
              "{\"event\":\"connecting\",\"endpoint\":\"" + wgEndpoint +
              "\",\"port\":" + String(wgEndpointPort) + "}");

  bool result = wg.begin(
      localIp,
      wgPrivateKey.c_str(),
      wgEndpoint.c_str(),
      wgPublicKey.c_str(),
      wgEndpointPort,
      wgDns.c_str(),
      wgAllowedIps.c_str());

  if (result)
  {
    wireguardConnected = true;
    addLogMessage("[WireGuard Task] Connected! Local IP: " + wgLocalIp);
    publishMqtt(mqttWgTaskTopic.c_str(),
                "{\"event\":\"connected\",\"local_ip\":\"" + wgLocalIp + "\"}");
  }
  else
  {
    wireguardConnected = false;
    addLogMessage("[WireGuard Task] Failed to connect");
    publishMqtt(mqttWgTaskTopic.c_str(), "{\"event\":\"connect_failed\"}");
  }

  // Main loop - monitor connection and reconnect if needed
  unsigned long lastReconnectAttempt = 0;
  while (true)
  {
    if (!wg.is_initialized())
    {
      wireguardConnected = false;

      // Attempt reconnect at interval
      if (millis() - lastReconnectAttempt >= WG_RECONNECT_INTERVAL)
      {
        lastReconnectAttempt = millis();

        // Only reconnect if network is available and time is valid
        if ((WiFi.status() == WL_CONNECTED || ethConnected) && time(nullptr) > 1000000)
        {
          addLogMessage("[WireGuard Task] Attempting reconnect...");
          publishMqtt(mqttWgTaskTopic.c_str(), "{\"event\":\"reconnecting\"}");

          result = wg.begin(
              localIp,
              wgPrivateKey.c_str(),
              wgEndpoint.c_str(),
              wgPublicKey.c_str(),
              wgEndpointPort,
              wgDns.c_str(),
              wgAllowedIps.c_str());

          if (result)
          {
            wireguardConnected = true;
            addLogMessage("[WireGuard Task] Reconnected!");
            publishMqtt(mqttWgTaskTopic.c_str(),
                        "{\"event\":\"reconnected\",\"local_ip\":\"" + wgLocalIp + "\"}");
          }
          else
          {
            addLogMessage("[WireGuard Task] Reconnect failed");
            publishMqtt(mqttWgTaskTopic.c_str(), "{\"event\":\"reconnect_failed\"}");
          }
        }
      }
    }
    else
    {
      wireguardConnected = true;
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}
