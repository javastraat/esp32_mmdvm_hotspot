/*
 * web_handlers_ota.cpp - OTA Firmware Update & Modem Flashing Routes
 *
 * Extracted from system_webserver.cpp to keep that file manageable.
 * All routes registered here use the global `server` object (extern WebServer server).
 */

#include "system/web_handlers_ota.h"
#include "system/system_webserver.h"   // extern WebServer server
#include "system/system_logger.h"      // addLogMessage()
#include "system/system_oled.h"        // displayRebootMessage(), oledTaskHandle
#include "system/system_firmware.h"    // handleSwitchPartition(), modemFirmwareVersion
#include "include/modem_flasher.h"     // modem flash functions and state globals
#include "include/config.h"            // OTA URLs, MMDVM_RX_PIN, OTA_TIMEOUT …
#include <HTTPClient.h>
#include <Update.h>

// Runtime MMDVM pin/baud variables (defined in esp32-rtos-mmdvm.ino)
extern int mmdvmRxPin;
extern int mmdvmTxPin;
extern int mmdvmBootPin;
extern int mmdvmResetPin;
extern int mmdvmWakeupPin;
extern int mmdvmBaudrate;
extern void saveSettings();

// ESP32 firmware update progress (defined in modem_flasher.cpp)
extern volatile bool espFirmwareUpdateActive;
extern volatile int espFirmwareUpdateProgress;
extern volatile unsigned long espFirmwareBytesTotal;
extern volatile unsigned long espFirmwareBytesWritten;
extern volatile bool espFirmwareReadyToFlash;

// OLED display flag: show "Firmware Ready" overlay (defined in system_oled.cpp)
extern volatile bool firmwareReadyMessageActive;

void registerOtaRoutes()
{
  // ---------------------------------------------------------------------------
  // ESP32 OTA — download firmware from URL, stage it, then flash on confirm
  // ---------------------------------------------------------------------------

  server.on("/download-update", HTTP_POST, []()
            {
    String version = server.hasArg("version") ? server.arg("version") : "stable";

    String downloadUrl;
    if (version == "beta") {
      downloadUrl = OTA_UPDATE_BETA_URL;
      addLogMessage("[OTA] Downloading BETA firmware from GitHub...");
    } else if (version == "factory") {
      downloadUrl = OTA_UPDATE_FACTORY_URL;
      addLogMessage("[OTA] Downloading Factory Setup firmware from GitHub...");
    } else if (version == "rtos") {
      downloadUrl = OTA_FIRMWARE_RTOS_URL_BETA;
      addLogMessage("[OTA] Downloading RTOS Development firmware...");
    } else {
      downloadUrl = OTA_UPDATE_URL;
      addLogMessage("[OTA] Downloading stable firmware from GitHub...");
    }

    addLogMessage("[OTA] URL: " + downloadUrl);

    espFirmwareUpdateActive = true;
    espFirmwareUpdateProgress = 0;
    espFirmwareBytesTotal = 0;
    espFirmwareBytesWritten = 0;

    int maxRetries = 3;
    bool downloadSuccess = false;
    String lastError = "";

    for (int attempt = 1; attempt <= maxRetries && !downloadSuccess; attempt++) {
      if (attempt > 1) {
        addLogMessage("[OTA] Retry attempt " + String(attempt) + " of " + String(maxRetries));
        delay(1000);
      }

      HTTPClient http;
      http.begin(downloadUrl);
      http.setTimeout(OTA_TIMEOUT);
      http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
      int httpCode = http.GET();

      addLogMessage("[OTA] HTTP Response: " + String(httpCode));

      if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        addLogMessage("[OTA] Content Length: " + String(contentLength) + " bytes");

        espFirmwareBytesTotal = contentLength;

        if (contentLength > 0) {
          if (!Update.begin(contentLength)) {
            lastError = "Not enough space for update";
            addLogMessage("[OTA] Update.begin failed: " + String(Update.getError()));
            http.end();
            continue;
          }

          WiFiClient* client = http.getStreamPtr();
          uint8_t buff[1024];
          size_t written = 0;
          unsigned long lastProgressMs = millis();
          bool writeError = false;

          while (http.connected() && (written < contentLength) && !writeError) {
            size_t available = client->available();
            if (available) {
              size_t toRead = min(available, sizeof(buff));
              size_t bytesRead = client->read(buff, toRead);
              if (bytesRead > 0) {
                if (Update.write(buff, bytesRead) != bytesRead) {
                  addLogMessage("[OTA] Write failed at " + String(written) + " bytes");
                  writeError = true;
                  break;
                }
                written += bytesRead;
                espFirmwareBytesWritten = written;
                espFirmwareUpdateProgress = (written * 100) / contentLength;

                if (millis() - lastProgressMs > 2000) {
                  addLogMessage("[OTA] Progress: " + String(written) + "/" + String(contentLength) + " bytes");
                  lastProgressMs = millis();
                }
              }
            }
            delay(1);
          }

          if (writeError) {
            Update.abort();
            lastError = "Write failed during download";
            http.end();
            continue;
          }

          addLogMessage("[OTA] Download complete. Written: " + String(written) + ", Expected: " + String(contentLength));

          if (written == contentLength) {
            if (Update.end(true)) {
              espFirmwareUpdateProgress = 100;
              delay(1000);
              espFirmwareUpdateActive = false;
              espFirmwareReadyToFlash = true;
              firmwareReadyMessageActive = true;
              addLogMessage("[OTA] Download successful: " + String(contentLength) + " bytes");
              server.send(200, "text/plain", "SUCCESS: Firmware ready for flash (" + String(contentLength) + " bytes)");
              http.end();
              downloadSuccess = true;
              return;
            } else {
              uint8_t error = Update.getError();
              String errorMsg = "Unknown error";
              switch (error) {
                case 1: errorMsg = "Flash write failed"; break;
                case 2: errorMsg = "Flash erase failed"; break;
                case 3: errorMsg = "Flash read failed"; break;
                case 4: errorMsg = "Bad partition"; break;
                case 5: errorMsg = "Bad size"; break;
                case 6: errorMsg = "Bad MD5"; break;
                case 7: errorMsg = "Insufficient space"; break;
                case 8: errorMsg = "Partition size mismatch"; break;
                case 9: errorMsg = "MD5 validation failed"; break;
              }
              Update.abort();
              lastError = errorMsg + " (code " + String(error) + ")";
              addLogMessage("[OTA] Update.end failed: Error " + String(error) + " - " + errorMsg);
            }
          } else {
            Update.abort();
            lastError = "Download incomplete (" + String(written) + "/" + String(contentLength) + " bytes)";
            addLogMessage("[OTA] Download incomplete: " + String(written) + "/" + String(contentLength));
          }
        } else {
          lastError = "Invalid content length";
          addLogMessage("[OTA] Invalid content length");
        }
      } else {
        lastError = "Failed to download (HTTP " + String(httpCode) + ")";
        addLogMessage("[OTA] Download failed, HTTP " + String(httpCode));
      }

      http.end();
    }

    espFirmwareUpdateActive = false;
    addLogMessage("[OTA] All " + String(maxRetries) + " download attempts failed");
    server.send(500, "text/plain", "ERROR: " + lastError + " - Failed after " + String(maxRetries) + " attempts"); });

  server.on("/flash-firmware", HTTP_POST, []()
            {
    addLogMessage("[OTA] Flashing firmware and rebooting...");
    server.send(200, "text/plain", "SUCCESS: Flashing firmware");

    espFirmwareReadyToFlash = false;

    displayRebootMessage();
    if (oledTaskHandle != NULL) {
      vTaskSuspend(oledTaskHandle);
    }

    delay(1000);
    ESP.restart(); });

  server.on("/cancel-flash", HTTP_POST, []()
            {
    espFirmwareReadyToFlash = false;
    espFirmwareUpdateActive = false;
    firmwareReadyMessageActive = false;
    server.send(200, "text/plain", "OK");
    addLogMessage("[OTA] Flash cancelled by user"); });

  server.on(
      "/upload-firmware", HTTP_POST, []()
      {
      if (Update.hasError()) {
        server.send(500, "text/plain", "ERROR: OTA Update Failed");
      } else {
        server.send(200, "text/plain", "SUCCESS: Firmware ready for flash (" + String(espFirmwareBytesTotal) + " bytes)");
      } },
      []()
      {
        static unsigned long espFirmwareExpectedSize = 0;
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START)
        {
          addLogMessage("[OTA] Starting firmware upload: " + String(upload.filename));
          espFirmwareUpdateActive = true;
          espFirmwareUpdateProgress = 0;
          espFirmwareBytesTotal = 0;
          espFirmwareBytesWritten = 0;
          espFirmwareExpectedSize = 0;
          if (server.hasArg("size")) {
            espFirmwareExpectedSize = server.arg("size").toInt();
            espFirmwareBytesTotal = espFirmwareExpectedSize;
          }
          if (!Update.begin(UPDATE_SIZE_UNKNOWN))
          {
            addLogMessage("[OTA] Error: Not enough space");
          }
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
          {
            addLogMessage("[OTA] Error: Write failed");
          }
          espFirmwareBytesWritten += upload.currentSize;
          if (espFirmwareExpectedSize > 0)
          {
            espFirmwareUpdateProgress = min(100, (int)((espFirmwareBytesWritten * 100) / espFirmwareExpectedSize));
          }
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
          espFirmwareUpdateProgress = 100;
          espFirmwareBytesTotal = upload.totalSize;
          espFirmwareBytesWritten = upload.totalSize;
          if (Update.end(true))
          {
            addLogMessage("[OTA] Upload SUCCESS! (" + String(upload.totalSize) + " bytes)");
          }
          else
          {
            addLogMessage("[OTA] Upload FAILED");
          }
          delay(1000);
          espFirmwareUpdateActive = false;
          espFirmwareReadyToFlash = true;
          firmwareReadyMessageActive = true;
        }
      });

  server.on("/switch-partition", HTTP_POST, []()
            {
    if (server.hasArg("partition")) {
      String partition = server.arg("partition");
      handleSwitchPartition(partition.c_str());
      server.send(200, "text/plain", "SUCCESS: Switching to " + partition);
    } else {
      server.send(400, "text/plain", "ERROR: No partition specified");
    } });

  // ---------------------------------------------------------------------------
  // Modem settings (serial pins / baud rate)
  // ---------------------------------------------------------------------------

  server.on("/api/reset-modem-settings", HTTP_POST, []()
            {
    mmdvmRxPin    = MMDVM_RX_PIN;
    mmdvmTxPin    = MMDVM_TX_PIN;
    mmdvmBootPin  = MMDVM_BOOT_PIN;
    mmdvmResetPin = MMDVM_RESET_PIN;
    mmdvmWakeupPin = -1;
    mmdvmBaudrate  = MMDVM_SERIAL_BAUD;
    saveSettings();
    server.send(200, "text/plain", "SUCCESS: Modem settings reset to defaults"); });

  server.on("/api/save-modem-settings", HTTP_POST, []()
            {
    if (!server.hasArg("rxpin") || !server.hasArg("txpin") || !server.hasArg("bootpin") ||
        !server.hasArg("resetpin") || !server.hasArg("wakeuppin") || !server.hasArg("baudrate")) {
      server.send(400, "text/plain", "ERROR: Missing one or more parameters");
      return;
    }
    mmdvmRxPin    = server.arg("rxpin").toInt();
    mmdvmTxPin    = server.arg("txpin").toInt();
    mmdvmBootPin  = server.arg("bootpin").toInt();
    mmdvmResetPin = server.arg("resetpin").toInt();
    mmdvmWakeupPin = server.arg("wakeuppin").toInt();
    mmdvmBaudrate  = server.arg("baudrate").toInt();
    saveSettings();
    server.send(200, "text/plain", "SUCCESS: Modem settings saved"); });

  server.on("/get-modem-version", HTTP_GET, []()
            {
    String version = modemFirmwareVersion;
    if (version == "Unknown" || version.length() == 0) {
      version = "Not detected";
    }
    server.send(200, "text/plain", version); });

  // ---------------------------------------------------------------------------
  // Modem flashing — URL-based (stream from HTTP directly to STM32)
  // ---------------------------------------------------------------------------

  server.on("/flash-modem-status", HTTP_GET, []()
            {
    String json = "{";
    json += "\"inProgress\":" + String(modemFlashInProgress ? "true" : "false") + ",";
    json += "\"progress\":" + String(modemFlashProgress) + ",";
    json += "\"status\":\"" + modemFlashStatus + "\"";
    json += "}";
    server.send(200, "application/json", json); });

  server.on("/flash-modem-url", HTTP_POST, []()
            {
    if (!server.hasArg("url")) {
      server.send(400, "text/plain", "ERROR: Missing URL parameter");
      return;
    }

    String url = server.arg("url");

    server.send(202, "text/plain", "Firmware flash started. Check status for progress.");

    initModemSerial();

    modemFlashInProgress = true;
    modemFlashProgress = 5;
    modemFlashStatus = "Connecting to server...";

    modemFirmwareUpdateActive = true;
    modemFirmwareUpdateProgress = 0;
    modemFirmwareBytesTotal = 0;
    modemFirmwareBytesWritten = 0;

    addLogMessage("[Modem] Downloading from: " + url);

    HTTPClient http;
    http.begin(url);
    http.setTimeout(30000);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      int totalLen = http.getSize();
      addLogMessage("[Modem] File size: " + String(totalLen) + " bytes");

      modemFirmwareBytesTotal = totalLen;

      modemFlashProgress = 10;
      modemFlashStatus = "Entering bootloader mode...";
      modemFirmwareUpdateProgress = 10;

      if (!modemInBootloaderMode) {
        modemEnterBootloader();
        delay(500);
      }

      if (!modemInBootloaderMode) {
        http.end();
        modemFlashInProgress = false;
        modemFlashStatus = "ERROR: Could not enter bootloader";
        return;
      }

      modemFlashProgress = 15;
      modemFlashStatus = "Syncing with bootloader...";

      if (!modemSyncBootloader()) {
        http.end();
        modemFlashInProgress = false;
        modemFlashStatus = "ERROR: Bootloader sync failed";
        return;
      }

      modemFlashProgress = 20;
      modemFlashStatus = "Erasing flash memory...";
      server.handleClient();

      if (!modemEraseFlash()) {
        http.end();
        modemFlashInProgress = false;
        modemFlashStatus = "ERROR: Flash erase failed";
        return;
      }

      modemFlashProgress = 30;
      modemFlashStatus = "Downloading and flashing...";
      server.handleClient();

      WiFiClient* stream = http.getStreamPtr();
      uint8_t buffer[MAX_WRITE_SIZE];
      size_t bufferPtr = 0;
      uint32_t address = FLASH_START_ADDR;
      size_t bytesWritten = 0;

      addLogMessage("[Modem] Starting download and flash...");

      while (http.connected() && (totalLen > 0 || totalLen == -1)) {
        size_t available = stream->available();
        if (available) {
          size_t toRead = min(available, MAX_WRITE_SIZE - bufferPtr);
          size_t readBytes = stream->read(buffer + bufferPtr, toRead);
          bufferPtr += readBytes;

          if (totalLen > 0) {
            totalLen -= readBytes;
          }

          if (bufferPtr >= MAX_WRITE_SIZE) {
            if (!modemWriteMemory(address, buffer, MAX_WRITE_SIZE)) {
              addLogMessage("[Modem] ERROR: Write failed at 0x" + String(address, HEX));
              http.end();
              modemFlashInProgress = false;
              modemFlashStatus = "ERROR: Write failed";
              return;
            }

            address += MAX_WRITE_SIZE;
            bytesWritten += MAX_WRITE_SIZE;
            bufferPtr = 0;

            int fileSize = http.getSize();
            if (fileSize > 0) {
              modemFlashProgress = 30 + ((bytesWritten * 60) / fileSize);
              modemFirmwareUpdateProgress = modemFlashProgress;
              modemFirmwareBytesWritten = bytesWritten;
            }

            if (bytesWritten % (16 * 256) == 0) {
              addLogMessage("[Modem] Progress: " + String(bytesWritten) + " bytes");
            }

            server.handleClient();
          }
        }

        server.handleClient();
        yield();
        delay(1);
      }

      if (bufferPtr > 0) {
        memset(buffer + bufferPtr, 0xFF, MAX_WRITE_SIZE - bufferPtr);
        if (!modemWriteMemory(address, buffer, MAX_WRITE_SIZE)) {
          addLogMessage("[Modem] ERROR: Final write failed");
          http.end();
          modemFlashInProgress = false;
          modemFlashStatus = "ERROR: Final write failed";
          return;
        }
        bytesWritten += bufferPtr;
      }

      modemFlashProgress = 95;
      modemFlashStatus = "Flash complete! Rebooting...";
      modemFirmwareUpdateProgress = 95;

      addLogMessage("[Modem] Download complete! " + String(bytesWritten) + " bytes written");

      http.end();

      for (int i = 0; i < 4; i++) {
        server.handleClient();
        delay(250);
      }

      modemFlashProgress = 100;
      modemFlashStatus = "Complete! Rebooting ESP32...";
      modemFirmwareUpdateProgress = 100;

      for (int i = 0; i < 2; i++) {
        server.handleClient();
        delay(250);
      }

      modemFirmwareUpdateActive = false;

      displayRebootMessage();
      if (oledTaskHandle != NULL) {
        vTaskSuspend(oledTaskHandle);
      }

      modemFlashInProgress = false;
      modemExitBootloader();

      addLogMessage("[SYSTEM] Rebooting ESP32 to reinitialize...");
      delay(500);
      ESP.restart();

    } else {
      addLogMessage("[Modem] ERROR: HTTP " + String(httpCode));
      http.end();
      modemFlashInProgress = false;
      modemFlashStatus = "ERROR: HTTP " + String(httpCode);
    } });

  // ---------------------------------------------------------------------------
  // Modem flashing — upload-based (buffer file, then flash)
  // ---------------------------------------------------------------------------

  server.on(
      "/flash-modem-upload", HTTP_POST, []()
      {
        if (modemFlashStatus.startsWith("ERROR")) {
          server.send(500, "text/plain", modemFlashStatus);
          if (modemFirmwareBuffer) { free(modemFirmwareBuffer); modemFirmwareBuffer = nullptr; }
          return;
        }

        if (!modemFirmwareBuffer || modemFirmwareSize == 0) {
          server.send(500, "text/plain", "ERROR: No firmware data received");
          return;
        }

        server.send(202, "text/plain", "Firmware upload complete. Flashing modem...");

        addLogMessage("[Modem] Starting modem flash from uploaded file (" + String(modemFirmwareSize) + " bytes)");

        initModemSerial();

        modemFlashInProgress = true;
        modemFlashProgress = 10;
        modemFlashStatus = "Entering bootloader mode...";
        modemFirmwareUpdateActive = true;
        modemFirmwareUpdateProgress = 10;
        modemFirmwareBytesTotal = modemFirmwareSize;
        modemFirmwareBytesWritten = 0;
        server.handleClient();

        if (!modemInBootloaderMode) {
          modemEnterBootloader();
          delay(500);
        }
        if (!modemInBootloaderMode) {
          modemFlashInProgress = false;
          modemFlashStatus = "ERROR: Could not enter bootloader";
          modemFirmwareUpdateActive = false;
          free(modemFirmwareBuffer); modemFirmwareBuffer = nullptr;
          return;
        }

        modemFlashProgress = 15;
        modemFlashStatus = "Syncing with bootloader...";
        server.handleClient();

        if (!modemSyncBootloader()) {
          modemFlashInProgress = false;
          modemFlashStatus = "ERROR: Bootloader sync failed";
          modemFirmwareUpdateActive = false;
          free(modemFirmwareBuffer); modemFirmwareBuffer = nullptr;
          return;
        }

        modemFlashProgress = 20;
        modemFlashStatus = "Erasing flash memory...";
        server.handleClient();

        if (!modemEraseFlash()) {
          modemFlashInProgress = false;
          modemFlashStatus = "ERROR: Flash erase failed";
          modemFirmwareUpdateActive = false;
          free(modemFirmwareBuffer); modemFirmwareBuffer = nullptr;
          return;
        }

        modemFlashProgress = 30;
        modemFlashStatus = "Flashing modem firmware...";
        server.handleClient();

        uint32_t address = FLASH_START_ADDR;
        size_t bytesWritten = 0;
        uint8_t writeBuffer[MAX_WRITE_SIZE];
        bool writeError = false;

        while (bytesWritten < modemFirmwareSize && !writeError) {
          size_t chunkSize = min((size_t)MAX_WRITE_SIZE, modemFirmwareSize - bytesWritten);
          memcpy(writeBuffer, modemFirmwareBuffer + bytesWritten, chunkSize);

          if (chunkSize < MAX_WRITE_SIZE) {
            memset(writeBuffer + chunkSize, 0xFF, MAX_WRITE_SIZE - chunkSize);
          }

          if (!modemWriteMemory(address, writeBuffer, MAX_WRITE_SIZE)) {
            addLogMessage("[Modem] ERROR: Write failed at 0x" + String(address, HEX));
            modemFlashInProgress = false;
            modemFlashStatus = "ERROR: Write failed";
            modemFirmwareUpdateActive = false;
            free(modemFirmwareBuffer); modemFirmwareBuffer = nullptr;
            return;
          }

          address += chunkSize;
          bytesWritten += chunkSize;

          modemFlashProgress = 30 + ((bytesWritten * 60) / modemFirmwareSize);
          modemFirmwareUpdateProgress = modemFlashProgress;
          modemFirmwareBytesWritten = bytesWritten;

          if (bytesWritten % (16 * 256) == 0) {
            addLogMessage("[Modem] Progress: " + String(bytesWritten) + " bytes");
            server.handleClient();
          }

          server.handleClient();
          yield();
        }

        free(modemFirmwareBuffer);
        modemFirmwareBuffer = nullptr;

        modemFlashProgress = 95;
        modemFlashStatus = "Finishing up...";
        modemFirmwareUpdateProgress = 95;
        addLogMessage("[Modem] Flash complete! " + String(bytesWritten) + " bytes written");

        for (int i = 0; i < 4; i++) {
          server.handleClient();
          delay(250);
        }

        modemFlashProgress = 100;
        modemFlashStatus = "Modem flash complete!";
        modemFirmwareUpdateProgress = 100;

        for (int i = 0; i < 2; i++) {
          server.handleClient();
          delay(250);
        }

        modemFlashInProgress = false;
        modemExitBootloader();

        modemFirmwareUpdateActive = false;
        espFirmwareReadyToFlash = true;
      },
      []()
      {
        HTTPUpload &upload = server.upload();

        if (upload.status == UPLOAD_FILE_START)
        {
          addLogMessage("[Modem] Receiving firmware file: " + String(upload.filename));
          modemFlashStatus = "Receiving firmware file...";
          modemFirmwareSize = 0;

          size_t expectedSize = 0;
          if (server.hasArg("size")) {
            expectedSize = server.arg("size").toInt();
          }
          if (expectedSize == 0 || expectedSize > 256 * 1024) {
            expectedSize = 256 * 1024;
          }
          if (modemFirmwareBuffer) { free(modemFirmwareBuffer); }
          modemFirmwareBuffer = (uint8_t*)malloc(expectedSize);
          if (!modemFirmwareBuffer) {
            modemFlashStatus = "ERROR: Not enough memory";
            addLogMessage("[Modem] ERROR: Failed to allocate " + String(expectedSize) + " bytes");
          }
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
          if (modemFirmwareBuffer && !modemFlashStatus.startsWith("ERROR")) {
            memcpy(modemFirmwareBuffer + modemFirmwareSize, upload.buf, upload.currentSize);
            modemFirmwareSize += upload.currentSize;
          }
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
          addLogMessage("[Modem] File received: " + String(modemFirmwareSize) + " bytes");
        }
        else if (upload.status == UPLOAD_FILE_ABORTED)
        {
          addLogMessage("[Modem] Upload aborted");
          if (modemFirmwareBuffer) { free(modemFirmwareBuffer); modemFirmwareBuffer = nullptr; }
        }
      });
}
