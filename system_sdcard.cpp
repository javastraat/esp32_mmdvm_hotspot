/*
 * SD Card System Module Implementation
 */

#include "system/system_sdcard.h"
#include "system/system_logger.h"
#include "system/system_mqtt.h"


extern String mqttSdCardTaskTopic; // MQTT topic for SD card task status updates

// Custom SPI instance for SD Card (HSPI)
SPIClass sdSPI(HSPI);

// SD Card mutex for thread-safe access
SemaphoreHandle_t sdCardMutex = NULL;

// SD Card status variables
bool sdCardMounted = false;
uint64_t sdCardSize = 0;
uint64_t sdCardUsed = 0;

TaskHandle_t sdCardTaskHandle = NULL;

void initSdCardTask()
{
  // Create mutex for SD card access
  sdCardMutex = xSemaphoreCreateMutex();
  if (sdCardMutex == NULL)
  {
    addLogMessage("[SD Card] ERROR: Failed to create mutex!");
    publishMqtt(mqttSdCardTaskTopic.c_str(), "{\"event\":\"mutex_failed\"}");
    return;
  }

  BaseType_t result = xTaskCreatePinnedToCore(
      sdCardTask,
      "SD Card Task",
      SDCARD_TASK_STACK,
      NULL,
      SDCARD_TASK_PRIORITY,
      &sdCardTaskHandle,
      0 // Run on core 0
  );
  if (result != pdPASS)
    log_e("[SD Card] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());
}

/*
 * TASK: SD Card Monitoring
 * Priority: Medium - Storage management
 *
 * This task handles:
 * - SD card initialization and mounting
 * - Creating database directory structure
 * - Monitoring SD card health
 * - Managing database files
 */
void sdCardTask(void *parameter)
{
  addLogMessage("[SD Card Task] Started");
  publishMqtt(mqttSdCardTaskTopic.c_str(), "{\"event\":\"started\"}");

  // Initialize SPI for SD Card with custom SPI instance
  sdSPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

  // Small delay to ensure SPI is ready
  vTaskDelay(100 / portTICK_PERIOD_MS);

  // Try to mount SD Card
  if (mountSDCard())
  {
    addLogMessage("[SD Card Task] SD Card mounted successfully");
    printSDCardInfo();

    // Create database directory if it doesn't exist
    if (createDatabaseDirectory())
    {
      addLogMessage("[SD Card Task] Database directory ready");
      publishMqtt(mqttSdCardTaskTopic.c_str(),
                  "{\"event\":\"db_dir_ready\",\"path\":\"" + String(SDCARD_DATABASE_DIR) + "\"}");
    }

    // Check for database files
    if (checkDatabaseFiles())
    {
      addLogMessage("[SD Card Task] Database files found");
      publishMqtt(mqttSdCardTaskTopic.c_str(), "{\"event\":\"db_files_found\"}");
    }
    else
    {
      addLogMessage("[SD Card Task] SQLite database not found - download required");
      publishMqtt(mqttSdCardTaskTopic.c_str(), "{\"event\":\"db_files_missing\"}");
    }
  }
  else
  {
    addLogMessage("[SD Card Task] Failed to mount SD Card");
    publishMqtt(mqttSdCardTaskTopic.c_str(), "{\"event\":\"mount_failed\"}");
  }

  // Monitor SD Card status
  static bool lastFilesExist = true; // Track previous state to avoid spam

  while (true)
  {
    if (sdCardMounted)
    {
      // Update SD card statistics
      uint8_t cardType = SD.cardType();
      if (cardType != CARD_NONE)
      {
        sdCardSize = SD.cardSize() / (1024 * 1024);  // Convert to MB
        sdCardUsed = SD.usedBytes() / (1024 * 1024); // Convert to MB

        // Check if SQLite database exists (CSV not needed for operations)
        bool filesExist = SD.exists(SDCARD_SQLITE_FILE);

        // Only print warning if state changed from present to missing
        if (!filesExist && lastFilesExist)
        {
          addLogMessage("[SD Card Task] Warning: SQLite database missing!");
          publishMqtt(mqttSdCardTaskTopic.c_str(), "{\"event\":\"db_missing_warning\"}");
        }
        lastFilesExist = filesExist;
      }
      else
      {
        addLogMessage("[SD Card Task] SD Card removed or disconnected");
        publishMqtt(mqttSdCardTaskTopic.c_str(), "{\"event\":\"card_removed\"}");
        sdCardMounted = false;
        lastFilesExist = false; // Reset state when card removed
      }
    }
    else
    {
      // Try to remount if card was removed
      addLogMessage("[SD Card Task] Attempting to remount SD Card...");
      publishMqtt(mqttSdCardTaskTopic.c_str(), "{\"event\":\"remounting\"}");
      if (mountSDCard())
      {
        addLogMessage("[SD Card Task] SD Card remounted successfully");
        printSDCardInfo();

        // Check for database files after remount
        if (checkDatabaseFiles())
        {
          addLogMessage("[SD Card Task] SQLite database found after remount");
          publishMqtt(mqttSdCardTaskTopic.c_str(), "{\"event\":\"db_files_found\"}");
          lastFilesExist = true;
        }
        else
        {
          publishMqtt(mqttSdCardTaskTopic.c_str(), "{\"event\":\"db_files_missing\"}");
          lastFilesExist = false;
        }
      }
    }

    vTaskDelay(SDCARD_CHECK_INTERVAL / portTICK_PERIOD_MS);
  }
}

/*
 * Mount SD Card
 */
bool mountSDCard()
{
  // Use the custom SPI instance
  if (!SD.begin(SD_CS_PIN, sdSPI))
  {
    addLogMessage("[SD Card Task] Card Mount Failed");
    publishMqtt(mqttSdCardTaskTopic.c_str(), "{\"event\":\"mount_failed\"}");
    sdCardMounted = false;
    return false;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE)
  {
    addLogMessage("[SD Card Task] No SD card attached");
    publishMqtt(mqttSdCardTaskTopic.c_str(), "{\"event\":\"no_card\"}");
    sdCardMounted = false;
    return false;
  }

  sdCardMounted = true;
  return true;
}

/*
 * Unmount SD Card
 */
void unmountSDCard()
{
  SD.end();
  sdCardMounted = false;
  addLogMessage("[SD Card Task] SD Card unmounted");
  publishMqtt(mqttSdCardTaskTopic.c_str(), "{\"event\":\"unmounted\"}");
}

/*
 * Create database directory structure
 */
bool createDatabaseDirectory()
{
  if (!sdCardMounted)
  {
    return false;
  }

  // Check if directory exists
  if (SD.exists(SDCARD_DATABASE_DIR))
  {
    addLogMessage("[SD Card Task] Database directory exists");
    publishMqtt(mqttSdCardTaskTopic.c_str(),
                "{\"event\":\"db_dir_exists\",\"path\":\"" + String(SDCARD_DATABASE_DIR) + "\"}");
    return true;
  }

  // Create directory
  if (SD.mkdir(SDCARD_DATABASE_DIR))
  {
    addLogMessage("[SD Card Task] Database directory created");
    publishMqtt(mqttSdCardTaskTopic.c_str(),
                "{\"event\":\"db_dir_created\",\"path\":\"" + String(SDCARD_DATABASE_DIR) + "\"}");
    return true;
  }
  else
  {
    addLogMessage("[SD Card Task] Failed to create database directory");
    publishMqtt(mqttSdCardTaskTopic.c_str(),
                "{\"event\":\"db_dir_create_failed\",\"path\":\"" + String(SDCARD_DATABASE_DIR) + "\"}");
    return false;
  }
}

/*
 * Check if database files exist (with detailed output)
 */
bool checkDatabaseFiles()
{
  if (!sdCardMounted)
  {
    return false;
  }

  // Only check SQLite database (CSV not needed for operations)
  bool sqliteExists = SD.exists(SDCARD_SQLITE_FILE);

  if (sqliteExists)
  {
    File sqliteFile = SD.open(SDCARD_SQLITE_FILE);
    if (sqliteFile)
    {
      addLogMessage("[SD Card Task] SQLite database: " + String(SDCARD_SQLITE_FILE) +
                    " (" + String(sqliteFile.size()) + " bytes)");
      publishMqtt(mqttSdCardTaskTopic.c_str(),
                  "{\"event\":\"db_file_info\",\"path\":\"" + String(SDCARD_SQLITE_FILE) +
                  "\",\"bytes\":" + String(sqliteFile.size()) + "}");
      sqliteFile.close();
    }
  }
  else
  {
    addLogMessage("[SD Card Task] SQLite database not found: " + String(SDCARD_SQLITE_FILE));
    publishMqtt(mqttSdCardTaskTopic.c_str(),
                "{\"event\":\"db_file_missing\",\"path\":\"" + String(SDCARD_SQLITE_FILE) + "\"}");
  }

  return sqliteExists;
}

/*
 * Print SD Card information
 */
void printSDCardInfo()
{
  if (!sdCardMounted)
  {
    addLogMessage("[SD Card Task] SD Card not mounted");
    publishMqtt(mqttSdCardTaskTopic.c_str(), "{\"event\":\"not_mounted\"}");
    return;
  }

  uint8_t cardType = SD.cardType();

  const char* cardTypeStr;
  switch (cardType)
  {
  case CARD_MMC:  cardTypeStr = "MMC";     break;
  case CARD_SD:   cardTypeStr = "SDSC";    break;
  case CARD_SDHC: cardTypeStr = "SDHC";   break;
  default:        cardTypeStr = "UNKNOWN"; break;
  }

  uint64_t cardSizeMB = SD.cardSize() / (1024 * 1024);
  uint64_t usedMB     = SD.usedBytes() / (1024 * 1024);
  uint64_t freeMB     = cardSizeMB - usedMB;

  addLogMessage("[SD Card Task] Card Type: " + String(cardTypeStr));
  addLogMessage("[SD Card Task] Total: " + String((uint32_t)cardSizeMB) + " MB"
                + "  Used: " + String((uint32_t)usedMB) + " MB"
                + "  Free: " + String((uint32_t)freeMB) + " MB");

  publishMqtt(mqttSdCardTaskTopic.c_str(),
              "{\"event\":\"card_info\""
              ",\"card_type\":\"" + String(cardTypeStr) + "\""
              ",\"size_mb\":" + String((uint32_t)cardSizeMB) +
              ",\"used_mb\":" + String((uint32_t)usedMB) +
              ",\"free_mb\":" + String((uint32_t)freeMB) + "}");
}
