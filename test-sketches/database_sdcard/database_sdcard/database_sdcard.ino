//
// api lookup from csv thru ip/api/dmr/user/?id=2041126


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

// Download progress tracking
volatile int downloadProgress = 0;  // 0-100%
volatile unsigned long downloadBytesTotal = 0;
volatile unsigned long downloadBytesWritten = 0;
String downloadStatus = "Idle";

// DMR search globals
volatile bool searchActive = false;
volatile bool searchRequested = false;
volatile unsigned long searchRadioId = 0;
String searchResult = "";
volatile int searchProgress = 0;

// Update checking
unsigned long remoteFileSize = 0;
unsigned long localFileSize = 0;
bool updateAvailable = false;

// SD card pins (adjust if your board uses different pins)
#define SPI_MISO_PIN 9
#define SPI_MOSI_PIN 11
#define SPI_SCLK_PIN 10
#define SD_CS_PIN 12

const char* ssid     = "TechInc";
const char* password = "itoldyoualready";

//const char* fileURL = "https://raw.githubusercontent.com/DMR-Database/dmr-database-appdata/refs/heads/main/radioid.json";
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

// Function to check if an update is available
bool checkForUpdate() {
  Serial.println("=== Checking for updates ===");

  // Get local file size
  localFileSize = 0;
  if (SD.exists(destFile)) {
    File localFile = SD.open(destFile);
    if (localFile) {
      localFileSize = localFile.size();
      localFile.close();
      Serial.print("Local file size: ");
      Serial.print(localFileSize);
      Serial.println(" bytes");
    }
  } else {
    Serial.println("Local file does not exist - update available!");
    updateAvailable = true;
    return true;
  }

  // Get remote file size using HEAD request
  HTTPClient http;
  http.begin(fileURL);
  int httpCode = http.sendRequest("HEAD");

  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND) {
    remoteFileSize = http.getSize();
    Serial.print("Remote file size: ");
    Serial.print(remoteFileSize);
    Serial.println(" bytes");

    // Compare sizes
    if (remoteFileSize != localFileSize) {
      Serial.println("Update available! File sizes differ.");
      updateAvailable = true;
    } else {
      Serial.println("No update needed. Files are the same size.");
      updateAvailable = false;
    }
  } else {
    Serial.print("Failed to check remote file. HTTP code: ");
    Serial.println(httpCode);
    updateAvailable = false;
  }

  http.end();
  Serial.println("=== Update check complete ===\n");
  return updateAvailable;
}

// Function to perform DMR search
void performDMRSearch() {
  Serial.println("=== Starting DMR user search ===");
  Serial.print("Searching for radio_id: ");
  Serial.println(searchRadioId);

  searchProgress = 0;
  searchResult = "";

  if (!SD.exists(destFile)) {
    Serial.println("ERROR: Database file not found");
    searchResult = "{\"error\":\"Database file not found\"}";
    return;
  }

  File dbFile = SD.open(destFile);
  if (!dbFile) {
    Serial.println("ERROR: Failed to open database file");
    searchResult = "{\"error\":\"Failed to open database file\"}";
    return;
  }

  unsigned long fileSize = dbFile.size();
  Serial.print("Database file size: ");
  Serial.print(fileSize);
  Serial.println(" bytes");

  // Search for the radio_id in the CSV file
  String searchStr = String(searchRadioId);
  bool found = false;

  // Larger buffer for better performance
  const int BUFFER_SIZE = 4096;
  char buffer[BUFFER_SIZE];
  String lineBuffer = "";
  lineBuffer.reserve(256);  // Pre-allocate for typical CSV line
  unsigned long bytesProcessed = 0;
  unsigned long startTime = millis();
  bool firstLine = true;  // Skip header line

  while (dbFile.available() && !found) {
    int bytesRead = dbFile.read((uint8_t*)buffer, BUFFER_SIZE);
    bytesProcessed += bytesRead;

    // Update progress
    searchProgress = (bytesProcessed * 100) / fileSize;

    // Progress indicator every 2MB
    if (bytesProcessed % 2000000 < BUFFER_SIZE) {
      Serial.print("Processed: ");
      Serial.print(bytesProcessed / 1024);
      Serial.print(" KB (");
      Serial.print(searchProgress);
      Serial.println("%)");
      yield();  // Allow other tasks to run
    }

    for (int i = 0; i < bytesRead && !found; i++) {
      char c = buffer[i];

      // Skip carriage returns
      if (c == '\r') continue;

      // Process line when we hit newline
      if (c == '\n') {
        if (firstLine) {
          // Skip header line: RADIO_ID,CALLSIGN,FIRST_NAME,CITY,STATE,COUNTRY
          firstLine = false;
          lineBuffer = "";
          continue;
        }

        if (lineBuffer.length() > 0) {
          // Parse CSV line: RADIO_ID,CALLSIGN,FIRST_NAME,CITY,STATE,COUNTRY
          int commaPos[5];  // Positions of the 5 commas
          int commaCount = 0;

          for (int j = 0; j < lineBuffer.length() && commaCount < 5; j++) {
            if (lineBuffer.charAt(j) == ',') {
              commaPos[commaCount++] = j;
            }
          }

          // Extract RADIO_ID (first field)
          if (commaCount >= 5) {
            String radioId = lineBuffer.substring(0, commaPos[0]);

            // Check if this is the radio_id we're looking for
            if (radioId == searchStr) {
              Serial.println("Found matching record!");
              Serial.println(lineBuffer);

              // Extract fields: RADIO_ID,CALLSIGN,FIRST_NAME,CITY,STATE,COUNTRY
              String callsign = lineBuffer.substring(commaPos[0] + 1, commaPos[1]);
              String firstName = lineBuffer.substring(commaPos[1] + 1, commaPos[2]);
              String city = lineBuffer.substring(commaPos[2] + 1, commaPos[3]);
              String state = lineBuffer.substring(commaPos[3] + 1, commaPos[4]);
              String country = lineBuffer.substring(commaPos[4] + 1);

              // Build response in the requested format
              searchResult = "{\"results\":[{";
              searchResult += "\"callsign\":\"" + callsign + "\",";
              searchResult += "\"city\":\"" + city + "\",";
              searchResult += "\"country\":\"" + country + "\",";
              searchResult += "\"name\":\"" + firstName + "\",";
              searchResult += "\"radio_id\":" + String(searchRadioId) + ",";
              searchResult += "\"state\":";
              if (state.length() > 0) {
                searchResult += "\"" + state + "\"";
              } else {
                searchResult += "null";
              }
              searchResult += "}]}";

              found = true;
            }
          }
        }

        lineBuffer = "";
      } else {
        lineBuffer += c;

        // Prevent buffer overflow (CSV lines shouldn't be this long)
        if (lineBuffer.length() > 400) {
          lineBuffer = "";
        }
      }
    }
  }

  // Check last line if file doesn't end with newline
  if (!found && lineBuffer.length() > 0 && !firstLine) {
    int commaPos[5];
    int commaCount = 0;

    for (int j = 0; j < lineBuffer.length() && commaCount < 5; j++) {
      if (lineBuffer.charAt(j) == ',') {
        commaPos[commaCount++] = j;
      }
    }

    if (commaCount >= 5) {
      String radioId = lineBuffer.substring(0, commaPos[0]);

      if (radioId == searchStr) {
        Serial.println("Found matching record!");
        Serial.println(lineBuffer);

        String callsign = lineBuffer.substring(commaPos[0] + 1, commaPos[1]);
        String firstName = lineBuffer.substring(commaPos[1] + 1, commaPos[2]);
        String city = lineBuffer.substring(commaPos[2] + 1, commaPos[3]);
        String state = lineBuffer.substring(commaPos[3] + 1, commaPos[4]);
        String country = lineBuffer.substring(commaPos[4] + 1);

        searchResult = "{\"results\":[{";
        searchResult += "\"callsign\":\"" + callsign + "\",";
        searchResult += "\"city\":\"" + city + "\",";
        searchResult += "\"country\":\"" + country + "\",";
        searchResult += "\"name\":\"" + firstName + "\",";
        searchResult += "\"radio_id\":" + String(searchRadioId) + ",";
        searchResult += "\"state\":";
        if (state.length() > 0) {
          searchResult += "\"" + state + "\"";
        } else {
          searchResult += "null";
        }
        searchResult += "}]}";

        found = true;
      }
    }
  }

  dbFile.close();

  unsigned long searchTime = millis() - startTime;
  Serial.print("Search completed in ");
  Serial.print(searchTime);
  Serial.println(" ms");

  if (!found) {
    Serial.println("No matching record found");
    searchResult = "{\"results\":[]}";
  }

  searchProgress = 100;
  Serial.println("=== DMR user search complete ===");
}

// Helper function to handle web client requests
void handleWebClient(WiFiClient &client) {
  String request = "";

  while (client.connected()) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      request += line + "\n";

      if (line == "\r") {  // End of HTTP header
        // Check request type
        if (request.indexOf("GET /api/dmr/user/?id=") >= 0) {
          // Extract radio_id from URL
          int idPos = request.indexOf("id=");
          if (idPos != -1) {
            idPos += 3; // Skip "id="
            int endPos = request.indexOf(' ', idPos);
            if (endPos == -1) endPos = request.indexOf('&', idPos);
            if (endPos == -1) endPos = request.indexOf('\r', idPos);

            String radioIdStr = request.substring(idPos, endPos);
            unsigned long radioId = radioIdStr.toInt();

            Serial.print("API request for radio_id: ");
            Serial.println(radioId);

            // Perform synchronous search
            searchRadioId = radioId;
            performDMRSearch();

            // Build complete response
            String jsonResponse = "{\"searching\":false,\"progress\":100,\"result\":" + searchResult + "}";

            // Send HTTP response with results
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println("Access-Control-Allow-Origin: *");
            client.print("Content-Length: ");
            client.println(jsonResponse.length());
            client.println();
            client.println(jsonResponse);

            // Clear result after sending
            searchResult = "";
          } else {
            client.println("HTTP/1.1 400 Bad Request");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.println("{\"error\":\"Missing id parameter\"}");
          }
        }
        else if (request.indexOf("GET /api/dmr/status") >= 0) {
          // Return current search status
          String jsonResponse;
          if (searchActive) {
            jsonResponse = "{\"searching\":true,\"progress\":" + String(searchProgress) + "}";
          } else if (searchResult.length() > 0) {
            jsonResponse = "{\"searching\":false,\"progress\":100,\"result\":" + searchResult + "}";
            // Clear the result after sending
            searchResult = "";
          } else {
            jsonResponse = "{\"searching\":false,\"progress\":0}";
          }

          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println("Connection: close");
          client.println("Access-Control-Allow-Origin: *");
          client.print("Content-Length: ");
          client.println(jsonResponse.length());
          client.println();
          client.println(jsonResponse);
        }
        else if (request.indexOf("GET / ") >= 0) {
          // Check for updates before serving the page
          checkForUpdate();

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
          client.println("button:hover{background:#0056b3}");
          client.println("button:disabled{background:#cccccc;cursor:not-allowed;opacity:0.6}");
          client.println(".progress-container{margin:15px 0;display:none}");
          client.println(".progress-bar{width:100%;background:#f0f0f0;border-radius:5px;overflow:hidden}");
          client.println(".progress-fill{height:30px;background:#4CAF50;width:0%;transition:width 0.3s;line-height:30px;color:white;text-align:center;font-weight:bold}");
          client.println(".status{padding:10px;background:#e3f2fd;border-radius:5px;margin:10px 0}</style>");
          client.println("<script>");
          client.println("var pollInterval;");
          client.println("function startDownload(){");
          client.println("  document.getElementById('progress-container').style.display='block';");
          client.println("  document.getElementById('download-btn').disabled=true;");
          client.println("  fetch('/download').then(()=>{");
          client.println("    pollInterval=setInterval(updateStatus,500);");
          client.println("  });");
          client.println("}");
          client.println("function updateStatus(){");
          client.println("  fetch('/status').then(r=>r.json()).then(data=>{");
          client.println("    console.log('Status:',data);");
          client.println("    document.getElementById('progress-fill').style.width=data.progress+'%';");
          client.println("    document.getElementById('progress-text').textContent=data.progress+'%';");
          client.println("    var mb=(data.bytesWritten/1024/1024).toFixed(2)+' / '+(data.bytesTotal/1024/1024).toFixed(2)+' MB';");
          client.println("    document.getElementById('bytes').textContent=mb;");
          client.println("    document.getElementById('status-text').textContent=data.status;");
          client.println("    if(!data.active && data.progress>=100){");
          client.println("      clearInterval(pollInterval);");
          client.println("      document.getElementById('download-btn').disabled=false;");
          client.println("      setTimeout(()=>{location.reload(true);},2000);");
          client.println("    }else if(!data.active && data.status.includes('ERROR')){");
          client.println("      clearInterval(pollInterval);");
          client.println("      document.getElementById('download-btn').disabled=false;");
          client.println("    }");
          client.println("  }).catch(e=>console.error('Fetch error:',e));");
          client.println("}");
          client.println("</script></head><body>");

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
          client.print("<p>Local file exists: ");
          client.println(SD.exists(destFile) ? "YES" : "NO");
          client.println("</p>");
          if (SD.exists(destFile)) {
            File dbFile = SD.open(destFile);
            if (dbFile) {
              client.print("<p>Local file size: ");
              client.print(dbFile.size());
              client.println(" bytes</p>");
              dbFile.close();
            }
          }

          // Show update status
          if (remoteFileSize > 0) {
            client.print("<p>Remote file size: ");
            client.print(remoteFileSize);
            client.println(" bytes</p>");

            if (updateAvailable) {
              client.println("<p style='color:#ff9800;font-weight:bold'>Update available!</p>");
            } else {
              client.println("<p style='color:#4CAF50;font-weight:bold'>Database is up to date</p>");
            }
          }

          client.print("<button id='download-btn' onclick='startDownload()'");
          if (updateAvailable) {
            client.print(" style='background:#4CAF50'");  // Green if update available
          }
          client.println(">Download Database</button>");
          if (SD.exists(destFile)) {
            client.println(" <button onclick=\"if(confirm('Delete database file?')) location.href='/delete'\" style=\"background:#dc3545\">Delete Database</button>");
          }
          client.println("<div id='progress-container' class='progress-container'>");
          client.println("<div class='status'>Status: <span id='status-text'>Starting...</span></div>");
          client.println("<div class='progress-bar'><div id='progress-fill' class='progress-fill'><span id='progress-text'>0%</span></div></div>");
          client.println("<p id='bytes'>0 / 0 MB</p>");
          client.println("</div>");
          client.println("</div>");

          client.println("</body></html>");
        }
        else if (request.indexOf("GET /status") >= 0) {
          // Return JSON status for AJAX polling
          String json = "{";
          json += "\"active\":";
          json += downloadActive ? "true" : "false";
          json += ",\"progress\":";
          json += String(downloadProgress);
          json += ",\"bytesWritten\":";
          json += String(downloadBytesWritten);
          json += ",\"bytesTotal\":";
          json += String(downloadBytesTotal);
          json += ",\"status\":\"";
          json += downloadStatus;
          json += "\"}";

          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println("Connection: close");
          client.print("Content-Length: ");
          client.println(json.length());
          client.println();
          client.println(json);
        }
        else if (request.indexOf("GET /download") >= 0) {
          // Trigger download
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("Connection: close");
          client.println();
          client.println("OK");

          downloadRequested = true;
        }
        else if (request.indexOf("GET /check") >= 0) {
          // Check for updates
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();
          client.println("<html><body><h1>Checking for Updates...</h1>");
          client.println("<p>Please wait...</p>");
          client.println("<script>setTimeout(function(){window.location.href='/';}, 2000);</script>");
          client.println("</body></html>");

          // Perform the update check (will happen after response is sent)
          client.stop();
          checkForUpdate();
          return;
        }
        else if (request.indexOf("GET /delete") >= 0) {
          // Delete database file
          bool deleteSuccess = false;

          if (SD.exists(destFile)) {
            if (SD.remove(destFile)) {
              deleteSuccess = true;
              Serial.println("Database file deleted via web interface");
            } else {
              Serial.println("Failed to delete database file");
            }
          }

          // Send response and redirect to main page
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();
          client.println("<html><body>");
          client.println("<script>window.location.href='/';</script>");
          client.println("</body></html>");
        }
        break;
      }
    }
  }

  delay(1);
  client.stop();
}

void performDownload() {
  Serial.println("=== Download requested ===");

  downloadProgress = 0;
  downloadBytesTotal = 0;
  downloadBytesWritten = 0;
  downloadStatus = "Connecting...";

  HTTPClient http;
  http.begin(fileURL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    downloadBytesTotal = http.getSize();
    Serial.print("Content-Length: ");
    Serial.print((unsigned long)downloadBytesTotal);
    Serial.println(" bytes");

    // Delete old file if exists
    if (SD.exists(destFile)) {
      SD.remove(destFile);
      Serial.println("Removed old database file");
    }

    File outFile = SD.open(destFile, FILE_WRITE);
    if (!outFile) {
      Serial.println("FAILED: Couldn't create file on SD!");
      http.end();
      downloadStatus = "ERROR: File open failed";
      downloadRequested = false;
      return;
    }
    Serial.println("File opened successfully!");

    // Start blinking LED
    downloadActive = true;
    xTaskCreatePinnedToCore(blinkTask, "BlinkTask", 1024, NULL, 1, &blinkTaskHandle, 1);

    downloadStatus = "Downloading...";
    unsigned long downloadStart = millis();

    // Manual download with progress tracking
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[2048];  // Larger buffer for better speed
    unsigned long totalWritten = 0;
    int lastPercent = -1;

    Serial.println("Starting download...");

    while (http.connected() && (totalWritten < downloadBytesTotal || downloadBytesTotal == -1)) {
      // Check for incoming web requests and handle status updates
      WiFiClient statusClient = server.available();
      if (statusClient) {
        handleWebClient(statusClient);
      }

      size_t available = stream->available();

      if (available) {
        int bytesToRead = min((int)available, (int)sizeof(buffer));
        if (downloadBytesTotal > 0) {
          bytesToRead = min(bytesToRead, (int)(downloadBytesTotal - totalWritten));
        }

        int bytesRead = stream->readBytes(buffer, bytesToRead);

        if (bytesRead > 0) {
          size_t bytesWritten = outFile.write(buffer, bytesRead);
          totalWritten += bytesWritten;
          downloadBytesWritten = totalWritten;

          // Update progress - use long arithmetic to avoid overflow
          int percent = 0;
          if (downloadBytesTotal > 0) {
            percent = (int)((totalWritten * 100UL) / downloadBytesTotal);
          }
          downloadProgress = percent;

          // Print progress every 10%
          if (percent != lastPercent && percent % 10 == 0) {
            Serial.printf("Progress: %d%% (%lu / %lu bytes)\n",
                         percent, totalWritten, downloadBytesTotal);
            lastPercent = percent;
          }
        }
        yield();  // Let other tasks run
      } else {
        delay(1);
      }

      // Safety check - if we've read enough, break
      if (downloadBytesTotal > 0 && totalWritten >= downloadBytesTotal) {
        break;
      }
    }

    unsigned long downloadDuration = millis() - downloadStart;

    // Stop blinking LED
    downloadActive = false;
    while (blinkTaskHandle != NULL) {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    outFile.close();

    downloadBytesWritten = totalWritten;
    downloadProgress = 100;
    downloadStatus = "Download complete!";

    Serial.printf("SUCCESS! Downloaded %lu bytes in %lu ms\n", totalWritten, downloadDuration);

    // Verify file was written
    if (SD.exists(destFile)) {
      File verifyFile = SD.open(destFile);
      if (verifyFile) {
        size_t fileSize = verifyFile.size();
        Serial.print("Verified file size on SD: ");
        Serial.print(fileSize);
        Serial.println(" bytes");

        if (fileSize == downloadBytesTotal) {
          Serial.println("File size matches!");
        } else {
          Serial.println("WARNING: File size mismatch!");
        }
        verifyFile.close();
      }
    }
  } else {
    Serial.print("HTTP GET failed: ");
    Serial.println(http.errorToString(httpCode));
    downloadStatus = "ERROR: HTTP " + String(httpCode);
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
    handleWebClient(client);
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