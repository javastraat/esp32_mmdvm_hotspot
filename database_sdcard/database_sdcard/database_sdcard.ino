#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <HTTPClient.h>

#define DL_LED_PIN 38  // GPIO 38 for download progress indication

// FreeRTOS blink task globals
TaskHandle_t blinkTaskHandle = NULL;
volatile bool downloadActive = false;

// SD card pins (adjust if your board uses different pins)
#define SPI_MISO_PIN 9
#define SPI_MOSI_PIN 11
#define SPI_SCLK_PIN 10
#define SD_CS_PIN 12

const char* ssid     = "TechInc";
const char* password = "itoldyoualready";

//const char* fileURL = "https://raw.githubusercontent.com/DMR-Database/database-beta/master/database.csv";
const char* fileURL = "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/main/database.csv";

//https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/database.csv
const char* databaseDir = "/database";
const char* destFile = "/database/database.csv";


SPIClass sdSPI(HSPI);

void listFiles(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    for (int i = 0; i < numTabs; i++) Serial.print('\t');
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      listFiles(entry, numTabs + 1);
    } else {
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void deleteRecursive(const char *path) {
  File entry = SD.open(path);
  if (!entry) return;
  if (entry.isDirectory()) {
    File file = entry.openNextFile();
    while (file) {
      String childPath = String(path) + "/" + String(file.name());
      deleteRecursive(childPath.c_str());
      file = entry.openNextFile();
    }
    entry.close();
    SD.rmdir(path);
    Serial.print("Directory deleted: "); Serial.println(path);
  } else {
    entry.close();
    SD.remove(path);
    Serial.print("File deleted: "); Serial.println(path);
  }
}

void showFirstLineWithString(const char* path, const char* search) {
  File csvFile = SD.open(path);
  if (csvFile) {
    Serial.print("Searching for "); Serial.print(search); Serial.println(" in database.csv:");
    const int BUFSZ = 1024;
    char buf[BUFSZ];
    String line = "";
    bool found = false;
    while (csvFile.available() && !found) {
      int toRead = min(csvFile.available(), BUFSZ);
      int r = csvFile.read((uint8_t*)buf, toRead);
      for (int i = 0; i < r && !found; ++i) {
        char c = buf[i];
        if (c == '\r') continue;
        if (c == '\n') {
          if (line.indexOf(search) != -1) {
            Serial.println(line);
            found = true;
          }
          line = "";
        } else {
          line += c;
        }
      }
    }
    if (!found && line.length() > 0 && line.indexOf(search) != -1) {
      Serial.println(line);
      found = true;
    }
    if (!found) Serial.println("(not found)");
    csvFile.close();
  } else {
    Serial.println("Could not open /database/database.csv to search for string.");
  }
}

void showFirst10Lines(const char* path) {
  File csvFile = SD.open(path);
  if (csvFile) {
    Serial.println("First 10 lines of database.csv:");
    const int N = 10;
    const int BUFSZ = 1024;
    char buf[BUFSZ];
    int lineCount = 0;
    String line = "";
    while (csvFile.available() && lineCount < N) {
      int toRead = min(csvFile.available(), BUFSZ);
      int r = csvFile.read((uint8_t*)buf, toRead);
      for (int i = 0; i < r && lineCount < N; ++i) {
        char c = buf[i];
        if (c == '\r') continue;
        if (c == '\n') {
          Serial.println(line);
          line = "";
          lineCount++;
        } else {
          line += c;
        }
      }
    }
    if (line.length() > 0 && lineCount < N) {
      Serial.println(line);
    }
    csvFile.close();
    Serial.println("...done showing CSV lines.");
  } else {
    Serial.println("Could not open /database/database.csv to read lines.");
  }
}

void showLast10Lines(const char* path) {
  File csvFile = SD.open(path);
  if (csvFile) {
    Serial.println("Last 10 lines of database.csv:");
    const int N = 10;
    String lastLines[N];
    int count = 0;
    String currentLine = "";
    while (csvFile.available()) {
      char c = csvFile.read();
      if (c == '\n' || c == '\r') {
        if (currentLine.length() > 0) {
          lastLines[count % N] = currentLine;
          count++;
          currentLine = "";
        }
      } else {
        currentLine += c;
      }
    }
    if (currentLine.length() > 0) { // Handle final line without newline
      lastLines[count % N] = currentLine;
      count++;
    }
    int start = (count > N) ? (count - N) : 0;
    int linesToPrint = min(count, N);
    for (int i = 0; i < linesToPrint; ++i) {
      Serial.println(lastLines[(start + i) % N]);
    }
    csvFile.close();
    Serial.println("...done showing last CSV lines.");
  } else {
    Serial.println("Could not open /database/database.csv to read lines.");
  }
}

// FreeRTOS blinking task: blinks LED while downloadActive == true
void blinkTask(void* param) {
  while (downloadActive) {
    digitalWrite(DL_LED_PIN, HIGH);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    digitalWrite(DL_LED_PIN, LOW);
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
  digitalWrite(DL_LED_PIN, LOW); // ensure off at end
  blinkTaskHandle = NULL;
  vTaskDelete(NULL);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 SD/WiFi File Demo");
  delay(1000);

  pinMode(DL_LED_PIN, OUTPUT);
  digitalWrite(DL_LED_PIN, LOW);

  // --- WiFi ---
  Serial.printf("Connecting to %s", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // --- SD Card ---
  sdSPI.begin(SPI_SCLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN);

  if (!SD.begin(SD_CS_PIN, sdSPI)) {
    Serial.println("SD Card Mount Failed!");
    while (1);
  }
  Serial.println("SD Card Mounted OK.");

  // --- Delete old file and macOS system folders if needed ---
  if (SD.exists("/test.txt")) { deleteRecursive("/test.txt"); }
  if (SD.exists("/owner.txt")) { deleteRecursive("/owner.txt"); }
  if (SD.exists("/.Spotlight-V100")) { deleteRecursive("/.Spotlight-V100"); }
  if (SD.exists("/.fseventsd")) { deleteRecursive("/.fseventsd"); }
  if (SD.exists("/.Trashes")) { deleteRecursive("/.Trashes"); }
  //if (SD.exists("/database")) { deleteRecursive("/database"); }

  // --- Write new owner.txt ---
  File dataFile = SD.open("/owner.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println("Property of PD2EMC");
    dataFile.println("This card contains ESP32 MMDVM Database and files");
    dataFile.close();
    Serial.println("New data written to owner.txt.");
  } else {
    Serial.println("Error opening owner.txt for writing!"); while (1);
  }

  File readFile = SD.open("/owner.txt");
  if (readFile) {
    Serial.println("Reading from owner.txt:");
    while (readFile.available()) { Serial.write(readFile.read()); }
    readFile.close();
    Serial.println("\n--- End of file ---");
  } else {
    Serial.println("Error opening owner.txt for reading!");
  }

  // --- Download file from internet and save to /database/database.csv ---
  if (!SD.exists(databaseDir)) {
    if (SD.mkdir(databaseDir)) {
      Serial.println("Created /database directory.");
    } else {
      Serial.println("Failed to create /database directory!");
      while (1);
    }
  }
  if(SD.exists(destFile)) {
    deleteRecursive(destFile);
    Serial.println("Deleted old /database/database.csv if existed.");
  }

  // --- List all files ---
  Serial.println("---- Files on SD Card ----");
  File root = SD.open("/");
  listFiles(root, 0);
  Serial.println("--------------------------");

  // --- Download from GitHub with blinking LED (FreeRTOS task) ---
  Serial.print("Downloading file from: ");
  Serial.println(fileURL);

  HTTPClient http;
  http.begin(fileURL);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    File outFile = SD.open(destFile, FILE_WRITE);
    if (!outFile) {
      Serial.println("Couldn't create file on SD.");
      http.end();
      while(1);
    }
    Serial.println("Download started (LED will blink while downloading, progress only after finish).");

    // Start blinking LED in parallel
    downloadActive = true;
    xTaskCreatePinnedToCore(
      blinkTask,        // Task function
      "BlinkTask",      // Name
      1024,             // Stack size
      NULL,             // Params
      1,                // Priority
      &blinkTaskHandle, // Handle
      1                 // Core
    );

    unsigned long downloadStart = millis();
    int written = http.writeToStream(&outFile);
    unsigned long downloadDuration = millis() - downloadStart;

    // End blinking LED
    downloadActive = false;
    // Wait for blink task to exit (optional: for short downloads, usually exits immediately)
    while (blinkTaskHandle != NULL) {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    outFile.close();
    Serial.printf("Download complete! File size: %d bytes\n", written);
    Serial.printf("Download took %lu ms\n", downloadDuration);
  } else {
    Serial.print("HTTP GET failed, error: ");
    Serial.println(http.errorToString(httpCode));
  }
  http.end();

  // --- List all files ---
  Serial.println("---- Files on SD Card ----");
  File rootlist = SD.open("/");
  listFiles(rootlist, 0);
  rootlist.close();
  Serial.println("--------------------------");

  // --- Show first line containing "PD2EMC" before first 10 lines ---
  showFirstLineWithString(destFile, "PD2EMC");
  showFirstLineWithString(destFile, "PD8JO");
  Serial.println("--------------------------");

  // --- Show first 10 lines of /database/database.csv ---
  showFirst10Lines(destFile);
  Serial.println("--------------------------");

  // --- Show last 10 lines of /database/database.csv ---
  showLast10Lines(destFile);
  Serial.println("--------------------------");
}

void loop() {
  // Nothing in loop for this example
}