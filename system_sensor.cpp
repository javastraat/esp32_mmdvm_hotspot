/*
 * Sensor System Module Implementation
 */

#include "system/system_sensor.h"
#include "system/system_webserver.h" // For accessing cpuUsage variable
#include "system/service_mqtt.h"
#include "system/system_logger.h"

extern String mqttSensorTaskTopic; // MQTT topic for sensor task status updates

TaskHandle_t sensorTaskHandle = NULL;

void initSensorTask()
{
  BaseType_t result = xTaskCreatePinnedToCore(
      sensorTask,
      "Sensor Task",
      SENSOR_TASK_STACK,
      NULL,
      SENSOR_TASK_PRIORITY,
      &sensorTaskHandle,
      0 // Run on core 0 (sensor monitoring is not time-critical)
  );
  if (result != pdPASS)
    log_e("[Sensor] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());
}

/*
 * TASK: Simulated Sensor Reading
 * Priority: Low - Background monitoring
 *
 * Simulates reading a sensor every 2 seconds
 * In a plain loop, you'd need millis() checks and careful delay management
 */
void sensorTask(void *parameter)
{
  addLogMessage("[Sensor Task] Started");
  publishMqtt(mqttSensorTaskTopic.c_str(), "{\"event\":\"started\"}");

  int reading = 0;

  while (true)
  {
    // Simulate sensor reading (would be real sensor in production)
    reading = random(20, 30);
    cpuUsage = (float)random(0, 100) / 100.0;

    addLogMessage("[Sensor Task] Temperature: " + String(reading) + "°C, CPU: " + String(cpuUsage * 100, 1) + "%");
    publishMqtt(mqttSensorTaskTopic.c_str(),
                "{\"event\":\"reading\",\"temperature_c\":" + String(reading) +
                ",\"cpu_pct\":" + String(cpuUsage * 100, 1) + "}");

    // Read every 2 seconds
    vTaskDelay(SENSOR_READ_INTERVAL / portTICK_PERIOD_MS);
  }
}
