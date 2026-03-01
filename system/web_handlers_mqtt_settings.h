/*
 * web_handlers_mqtt_settings.h - MQTT Settings Routes Registration
 *
 * Declares registerMqttSettingsRoutes() which registers HTTP routes for:
  *   /api/save-mqtt-service + reset
  *   /api/save-mqtt-broker + reset
  *   /api/save-mqtt-auth + reset
  *   /api/save-mqtt-topics + reset
  *   /api/save-mqtt-advanced + reset
 *   /api/get-mqtt-token
 *   /api/save-mqtt-token
 */

#ifndef WEB_HANDLERS_MQTT_SETTINGS
#define WEB_HANDLERS_MQTT_SETTINGS

void registerMqttSettingsRoutes();

#endif // WEB_HANDLERS_MQTT_SETTINGS
