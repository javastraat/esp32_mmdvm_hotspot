#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <HTTPClient.h>

#define DL_LED_PIN 38  // GPIO 38 for download progress indication

// Simple web server
WiFiServer server(80);

// FreeRTOS blink task globals
TaskHandle_t blinkTaskHandle = NULL;
volatile bool downloadActive = false;
volatile bool downloadRequested = false;

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

  // --- Ensure /database directory exists (but don't auto-download) ---
  if (!SD.exists(databaseDir)) {
    if (SD.mkdir(databaseDir)) {
      Serial.println("Created /database directory.");
    } else {
      Serial.println("Failed to create /database directory!");
    }
  }

  // --- List all files ---
  Serial.println("---- Files on SD Card ----");
  File root = SD.open("/");
  listFiles(root, 0);
  root.close();
  Serial.println("--------------------------");

  // Start web server
  server.begin();
  Serial.println("Web server started on port 80");
  Serial.print("Access at: http://");
  Serial.println(WiFi.localIP());
}

void performDownload() {
  Serial.println("=== Download requested ===");

  HTTPClient http;
  http.begin(fileURL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    // Delete old file if exists
    if (SD.exists(destFile)) {
      SD.remove(destFile);
      Serial.println("Removed old database file");
    }

    File outFile = SD.open(destFile, FILE_WRITE);
    if (!outFile) {
      Serial.println("FAILED: Couldn't create file on SD!");
      http.end();
      return;
    }
    Serial.println("File opened successfully!");

    // Start blinking LED
    downloadActive = true;
    xTaskCreatePinnedToCore(blinkTask, "BlinkTask", 1024, NULL, 1, &blinkTaskHandle, 1);

    unsigned long downloadStart = millis();
    int written = http.writeToStream(&outFile);
    unsigned long downloadDuration = millis() - downloadStart;

    // Stop blinking LED
    downloadActive = false;
    while (blinkTaskHandle != NULL) {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    outFile.close();
    Serial.printf("SUCCESS! Downloaded %d bytes in %lu ms\n", written, downloadDuration);
  } else {
    Serial.print("HTTP GET failed: ");
    Serial.println(http.errorToString(httpCode));
  }
  http.end();
  downloadRequested = false;
}

void loop() {
  // Handle download requests from main loop
  if (downloadRequested) {
    performDownload();
  }

  // Handle web server requests
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected");
    String request = "";

    while (client.connected()) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        request += line + "\n";

        if (line == "\r") {  // End of HTTP header
          // Check request type
          if (request.indexOf("GET / ") >= 0) {
            // Send HTML page
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();

            client.println("<!DOCTYPE html><html><head>");
            client.println("<title>SD Card Test</title>");
            client.println("<style>body{font-family:Arial;margin:20px}");
            client.println(".card{border:1px solid #ddd;padding:15px;margin:10px 0;border-radius:5px}");
            client.println("button{padding:10px 20px;background:#007bff;color:white;border:none;border-radius:4px;cursor:pointer}");
            client.println("button:hover{background:#0056b3}</style></head><body>");

            client.println("<h1>SD Card Test Interface</h1>");

            // Card 1: SD Card Status
            client.println("<div class='card'><h2>SD Card Status</h2>");
            client.print("<p>Card Type: ");
            uint8_t cardType = SD.cardType();
            client.print(cardType == CARD_SDHC ? "SDHC" : cardType == CARD_SD ? "SD" : "Unknown");
            client.println("</p>");
            client.print("<p>Card Size: ");
            client.print((uint32_t)(SD.cardSize() / (1024 * 1024)));
            client.println(" MB</p></div>");

            // Card 2: File List
            client.println("<div class='card'><h2>Files on SD Card</h2><pre>");
            File root = SD.open("/");
            listFilesHTML(root, 0, client);
            root.close();
            client.println("</pre></div>");

            // Card 3: Download Button
            client.println("<div class='card'><h2>Database Download</h2>");
            client.print("<p>Database file exists: ");
            client.println(SD.exists(destFile) ? "YES" : "NO");
            client.println("</p>");
            if (SD.exists(destFile)) {
              File dbFile = SD.open(destFile);
              if (dbFile) {
                client.print("<p>File size: ");
                client.print(dbFile.size());
                client.println(" bytes</p>");
                dbFile.close();
              }
            }
            client.println("<button onclick=\"location.href='/download'\">Download Database from GitHub</button>");
            client.println("</div>");

            client.println("</body></html>");
          }
          else if (request.indexOf("GET /download") >= 0) {
            // Trigger download
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            client.println("<html><body><h1>Download Started!</h1>");
            client.println("<p>Check Serial Monitor for progress...</p>");
            client.println("<p><a href='/'>Back to Status</a></p></body></html>");

            downloadRequested = true;
          }
          break;
        }
      }
    }

    delay(1);
    client.stop();
    Serial.println("Client disconnected");
  }
}

// Helper function to list files as HTML
void listFilesHTML(File dir, int numTabs, WiFiClient &client) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;

    for (int i = 0; i < numTabs; i++) client.print("  ");
    client.print(entry.name());

    if (entry.isDirectory()) {
      client.println("/");
      listFilesHTML(entry, numTabs + 1, client);
    } else {
      client.print(" (");
      client.print(entry.size());
      client.println(" bytes)");
    }
    entry.close();
  }
}