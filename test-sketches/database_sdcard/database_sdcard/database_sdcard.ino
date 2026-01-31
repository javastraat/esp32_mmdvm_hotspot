//
// api lookup from csv thru ip/api/dmr/user/?id=1234567 or sqlite ip/api/sqlite/search?field=radioid&value=1234567


#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <sqlite3.h>

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

// Update checking for CSV
unsigned long remoteFileSize = 0;
unsigned long localFileSize = 0;
bool updateAvailable = false;

// Update checking for SQLite DB
unsigned long remoteFileSize_db = 0;
unsigned long localFileSize_db = 0;
bool updateAvailable_db = false;

// SQLite database download tracking
volatile bool downloadActive_db = false;
volatile bool downloadRequested_db = false;
volatile int downloadProgress_db = 0;
volatile unsigned long downloadBytesTotal_db = 0;
volatile unsigned long downloadBytesWritten_db = 0;
String downloadStatus_db = "Idle";

// SQLite search globals
sqlite3 *db = NULL;
String sqliteSearchResult = "";
volatile bool sqliteSearchActive = false;

// SD card pins (adjust if your board uses different pins)
#define SPI_MISO_PIN 9
#define SPI_MOSI_PIN 11
#define SPI_SCLK_PIN 10
#define SD_CS_PIN 12
// End SD card pins
//
// WiFi credentials
const char* ssid     = "TechInc";
const char* password = "itoldyoualready";
// End WiFi credentials
//
// Database file paths and URLs
// Database directory
const char* databaseDir = "/database";
// Database csv file info
const char* destFile = "/database/radioid.csv";
const char* fileURL = "https://raw.githubusercontent.com/DMR-Database/dmr-database-appdata/refs/heads/main/radioid.csv";
// Database sqlite file info
const char* destFile_db = "/database/esp32_database.db";
// Server file URL for sqlite database
//const char* fileURL_db = "https://raw.githubusercontent.com/DMR-Database/dmr-database-appdata/refs/heads/main/radio_database.db";
// Local server for testing
const char* fileURL_db = "http://192.168.2.173/dmr-database/esp32_database.db";
// End database file info

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
  if (SD.exists("/database/database.json")) { deleteRecursive("/database/database.json"); }
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

// Function to check if an update is available for CSV
bool checkForUpdate() {
  Serial.println("=== Checking for updates ===");

  // Get local file size
  localFileSize = 0;
  bool localExists = SD.exists(destFile);
  if (localExists) {
    File localFile = SD.open(destFile);
    if (localFile) {
      localFileSize = localFile.size();
      localFile.close();
      Serial.print("Local file size: ");
      Serial.print(localFileSize);
      Serial.println(" bytes");
    }
  } else {
    Serial.println("Local file does not exist");
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

    // Compare sizes or check if local doesn't exist
    if (!localExists || remoteFileSize != localFileSize) {
      Serial.println("Update available!");
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

// Function to check if an update is available for SQLite DB
bool checkForUpdate_db() {
  Serial.println("=== Checking for SQLite DB updates ===");

  // Get local file size
  localFileSize_db = 0;
  bool localExists = SD.exists(destFile_db);
  if (localExists) {
    File localFile = SD.open(destFile_db);
    if (localFile) {
      localFileSize_db = localFile.size();
      localFile.close();
      Serial.print("Local DB file size: ");
      Serial.print(localFileSize_db);
      Serial.println(" bytes");
    }
  } else {
    Serial.println("Local DB file does not exist");
  }

  // Get remote file size using HEAD request
  HTTPClient http;
  http.begin(fileURL_db);
  int httpCode = http.sendRequest("HEAD");

  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND) {
    remoteFileSize_db = http.getSize();
    Serial.print("Remote DB file size: ");
    Serial.print(remoteFileSize_db);
    Serial.println(" bytes");

    // Compare sizes or check if local doesn't exist
    if (!localExists || remoteFileSize_db != localFileSize_db) {
      Serial.println("DB Update available!");
      updateAvailable_db = true;
    } else {
      Serial.println("No DB update needed. Files are the same size.");
      updateAvailable_db = false;
    }
  } else {
    Serial.print("Failed to check remote DB file. HTTP code: ");
    Serial.println(httpCode);
    updateAvailable_db = false;
  }

  http.end();
  Serial.println("=== DB Update check complete ===\n");
  return updateAvailable_db;
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

            // Send HTTP response with results (searchResult already contains the complete JSON)
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println("Access-Control-Allow-Origin: *");
            client.print("Content-Length: ");
            client.println(searchResult.length());
            client.println();
            client.println(searchResult);

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
          checkForUpdate_db();

          // Send HTML page
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();

          client.println("<!DOCTYPE html><html><head>");
          client.println("<title>ESP32 DMR Database</title>");
          client.println("<style>body{font-family:Arial;margin:20px}");
          client.println(".card{border:1px solid #ddd;padding:15px;margin:10px 0;border-radius:5px}");
          client.println("button{padding:10px 20px;background:#007bff;color:white;border:none;border-radius:4px;cursor:pointer;margin:2px}");
          client.println("button:hover{background:#0056b3}");
          client.println("button:disabled{background:#cccccc;cursor:not-allowed;opacity:0.6}");
          client.println(".progress-container{margin:15px 0;display:none}");
          client.println(".progress-bar{width:100%;background:#f0f0f0;border-radius:5px;overflow:hidden}");
          client.println(".progress-fill{height:30px;background:#4CAF50;width:0%;transition:width 0.3s;line-height:30px;color:white;text-align:center;font-weight:bold}");
          client.println(".status{padding:10px;background:#e3f2fd;border-radius:5px;margin:10px 0}");
          client.println("select,input[type=text]{padding:8px;border:1px solid #ddd;border-radius:3px}</style>");
          client.println("<script>");
          // CSV download functions
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
          // SQLite DB download functions
          client.println("var pollIntervalDB;");
          client.println("function startDownloadDB(){");
          client.println("  document.getElementById('progress-container-db').style.display='block';");
          client.println("  document.getElementById('download-btn-db').disabled=true;");
          client.println("  fetch('/download_db').then(()=>{");
          client.println("    pollIntervalDB=setInterval(updateStatusDB,500);");
          client.println("  });");
          client.println("}");
          client.println("function updateStatusDB(){");
          client.println("  fetch('/status_db').then(r=>r.json()).then(data=>{");
          client.println("    document.getElementById('progress-fill-db').style.width=data.progress+'%';");
          client.println("    document.getElementById('progress-text-db').textContent=data.progress+'%';");
          client.println("    var mb=(data.bytesWritten/1024/1024).toFixed(2)+' / '+(data.bytesTotal/1024/1024).toFixed(2)+' MB';");
          client.println("    document.getElementById('bytes-db').textContent=mb;");
          client.println("    document.getElementById('status-text-db').textContent=data.status;");
          client.println("    if(!data.active && data.progress>=100){");
          client.println("      clearInterval(pollIntervalDB);");
          client.println("      document.getElementById('download-btn-db').disabled=false;");
          client.println("      setTimeout(()=>{location.reload(true);},2000);");
          client.println("    }else if(!data.active && data.status.includes('ERROR')){");
          client.println("      clearInterval(pollIntervalDB);");
          client.println("      document.getElementById('download-btn-db').disabled=false;");
          client.println("    }");
          client.println("  }).catch(e=>console.error('DB Fetch error:',e));");
          client.println("}");
          // CSV search function
          client.println("function searchRadioId(){");
          client.println("  var id=document.getElementById('search-id').value;");
          client.println("  if(!id){alert('Please enter a Radio ID');return;}");
          client.println("  document.getElementById('search-btn').disabled=true;");
          client.println("  document.getElementById('search-result').innerHTML='<p>Searching...</p>';");
          client.println("  fetch('/api/dmr/user/?id='+id).then(r=>r.json()).then(data=>{");
          client.println("    document.getElementById('search-btn').disabled=false;");
          client.println("    if(data.results && data.results.length>0){");
          client.println("      var r=data.results[0];");
          client.println("      var html='<table style=\"width:100%;border-collapse:collapse\">';");
          client.println("      html+='<tr><td style=\"padding:5px;border:1px solid #ddd;font-weight:bold\">Callsign:</td><td style=\"padding:5px;border:1px solid #ddd\">'+r.callsign+'</td></tr>';");
          client.println("      html+='<tr><td style=\"padding:5px;border:1px solid #ddd;font-weight:bold\">Name:</td><td style=\"padding:5px;border:1px solid #ddd\">'+r.name+'</td></tr>';");
          client.println("      html+='<tr><td style=\"padding:5px;border:1px solid #ddd;font-weight:bold\">City:</td><td style=\"padding:5px;border:1px solid #ddd\">'+r.city+'</td></tr>';");
          client.println("      html+='<tr><td style=\"padding:5px;border:1px solid #ddd;font-weight:bold\">State:</td><td style=\"padding:5px;border:1px solid #ddd\">'+(r.state||'N/A')+'</td></tr>';");
          client.println("      html+='<tr><td style=\"padding:5px;border:1px solid #ddd;font-weight:bold\">Country:</td><td style=\"padding:5px;border:1px solid #ddd\">'+r.country+'</td></tr>';");
          client.println("      html+='<tr><td style=\"padding:5px;border:1px solid #ddd;font-weight:bold\">Radio ID:</td><td style=\"padding:5px;border:1px solid #ddd\">'+r.radio_id+'</td></tr>';");
          client.println("      html+='</table>';");
          client.println("      document.getElementById('search-result').innerHTML=html;");
          client.println("    }else{");
          client.println("      document.getElementById('search-result').innerHTML='<p style=\"color:#dc3545\">No results found for Radio ID: '+id+'</p>';");
          client.println("    }");
          client.println("  }).catch(e=>{");
          client.println("    document.getElementById('search-btn').disabled=false;");
          client.println("    document.getElementById('search-result').innerHTML='<p style=\"color:#dc3545\">Error: '+e.message+'</p>';");
          client.println("  });");
          client.println("}");
          // SQLite advanced search function
          client.println("function searchSQLite(){");
          client.println("  var field=document.getElementById('search-field').value;");
          client.println("  var value=document.getElementById('search-value').value;");
          client.println("  if(!value){alert('Please enter a search value');return;}");
          client.println("  document.getElementById('sqlite-search-btn').disabled=true;");
          client.println("  document.getElementById('sqlite-result').innerHTML='<p>Searching SQLite database...</p>';");
          client.println("  fetch('/api/sqlite/search?field='+field+'&value='+encodeURIComponent(value)).then(r=>r.json()).then(data=>{");
          client.println("    document.getElementById('sqlite-search-btn').disabled=false;");
          client.println("    if(data.error){");
          client.println("      document.getElementById('sqlite-result').innerHTML='<p style=\"color:#dc3545\">Error: '+data.error+'</p>';");
          client.println("      return;");
          client.println("    }");
          client.println("    if(data.results && data.results.length>0){");
          client.println("      var html='<p>Found '+data.results.length+' result(s):</p>';");
          client.println("      html+='<table style=\"width:100%;border-collapse:collapse\">';");
          client.println("      html+='<tr style=\"background:#f0f0f0\"><th style=\"padding:5px;border:1px solid #ddd\">Radio ID</th><th style=\"padding:5px;border:1px solid #ddd\">Callsign</th><th style=\"padding:5px;border:1px solid #ddd\">Name</th><th style=\"padding:5px;border:1px solid #ddd\">City</th><th style=\"padding:5px;border:1px solid #ddd\">State</th><th style=\"padding:5px;border:1px solid #ddd\">Country</th></tr>';");
          client.println("      for(var i=0;i<data.results.length;i++){");
          client.println("        var r=data.results[i];");
          client.println("        html+='<tr>';");
          client.println("        html+='<td style=\"padding:5px;border:1px solid #ddd\">'+(r.radio_id||r.RADIO_ID||'N/A')+'</td>';");
          client.println("        html+='<td style=\"padding:5px;border:1px solid #ddd\">'+(r.callsign||r.CALLSIGN||'N/A')+'</td>';");
          client.println("        html+='<td style=\"padding:5px;border:1px solid #ddd\">'+(r.first_name||r.FIRST_NAME||'N/A')+'</td>';");
          client.println("        html+='<td style=\"padding:5px;border:1px solid #ddd\">'+(r.city||r.CITY||'N/A')+'</td>';");
          client.println("        html+='<td style=\"padding:5px;border:1px solid #ddd\">'+(r.state||r.STATE||'N/A')+'</td>';");
          client.println("        html+='<td style=\"padding:5px;border:1px solid #ddd\">'+(r.country||r.COUNTRY||'N/A')+'</td>';");
          client.println("        html+='</tr>';");
          client.println("      }");
          client.println("      html+='</table>';");
          client.println("      document.getElementById('sqlite-result').innerHTML=html;");
          client.println("    }else{");
          client.println("      document.getElementById('sqlite-result').innerHTML='<p style=\"color:#dc3545\">No results found</p>';");
          client.println("    }");
          client.println("  }).catch(e=>{");
          client.println("    document.getElementById('sqlite-search-btn').disabled=false;");
          client.println("    document.getElementById('sqlite-result').innerHTML='<p style=\"color:#dc3545\">Error: '+e.message+'</p>';");
          client.println("  });");
          client.println("}");
          // Custom delete function
          client.println("function deleteCustomPath(){");
          client.println("  var path=document.getElementById('delete-path').value;");
          client.println("  if(!path){alert('Please enter a file or directory path');return;}");
          client.println("  if(!path.startsWith('/')){alert('Path must start with /');return;}");
          client.println("  if(path==='/' || path==='/database'){alert('Cannot delete root or database directory');return;}");
          client.println("  if(!confirm('Are you sure you want to delete: '+path+'?\\n\\nThis action cannot be undone!')){return;}");
          client.println("  document.getElementById('delete-custom-btn').disabled=true;");
          client.println("  document.getElementById('delete-result').innerHTML='<p style=\"color:#ff9800\">Deleting...</p>';");
          client.println("  fetch('/delete_custom?path='+encodeURIComponent(path)).then(r=>r.json()).then(data=>{");
          client.println("    document.getElementById('delete-custom-btn').disabled=false;");
          client.println("    if(data.success){");
          client.println("      document.getElementById('delete-result').innerHTML='<p style=\"color:#4CAF50\">'+data.message+'</p>';");
          client.println("      document.getElementById('delete-path').value='';");
          client.println("      setTimeout(()=>{location.reload();},1500);");
          client.println("    }else{");
          client.println("      document.getElementById('delete-result').innerHTML='<p style=\"color:#dc3545\">Error: '+data.message+'</p>';");
          client.println("    }");
          client.println("  }).catch(e=>{");
          client.println("    document.getElementById('delete-custom-btn').disabled=false;");
          client.println("    document.getElementById('delete-result').innerHTML='<p style=\"color:#dc3545\">Error: '+e.message+'</p>';");
          client.println("  });");
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

          // Card 3: Owner Info
          client.println("<div class='card'><h2>Owner Information</h2>");
          if (SD.exists("/owner.txt")) {
            File ownerFile = SD.open("/owner.txt");
            if (ownerFile) {
              client.println("<pre style='margin:0;background:#f5f5f5;padding:10px;border-radius:3px'>");
              while (ownerFile.available()) {
                client.write(ownerFile.read());
              }
              ownerFile.close();
              client.println("</pre>");
            } else {
              client.println("<p>Could not read owner.txt</p>");
            }
          } else {
            client.println("<p>owner.txt not found</p>");
          }
          client.println("</div>");

          // Card 4: DMR Database Search (CSV)
          client.println("<div class='card'><h2>DMR Database Search (CSV)</h2>");
          if (SD.exists(destFile)) {
            client.println("<p>Search for DMR user by Radio ID:</p>");
            client.println("<div style='margin:10px 0'>");
            client.println("<input type='text' id='search-id' placeholder='Enter Radio ID' style='padding:8px;width:200px;border:1px solid #ddd;border-radius:3px'>");
            client.println(" <button id='search-btn' onclick='searchRadioId()'>Search</button>");
            client.println("</div>");
            client.println("<div id='search-result' style='margin-top:15px'></div>");
          } else {
            client.println("<p style='color:#ff9800;font-weight:bold'>CSV Database not available</p>");
            client.println("<p>Please download the CSV database first to enable search functionality.</p>");
          }
          client.println("</div>");

          // Card 5: Advanced SQLite Search
          client.println("<div class='card'><h2>Advanced Database Search (SQLite)</h2>");
          if (SD.exists(destFile_db)) {
            client.println("<p>Search the full SQLite database by different fields:</p>");
            client.println("<div style='margin:10px 0'>");
            client.println("<select id='search-field' style='width:120px'>");
            client.println("<option value='radio_id'>Radio ID</option>");
            client.println("<option value='callsign'>Callsign</option>");
            client.println("<option value='name'>Name</option>");
            client.println("<option value='city'>City</option>");
            client.println("<option value='state'>State</option>");
            client.println("<option value='country'>Country</option>");
            client.println("</select>");
            client.println(" <input type='text' id='search-value' placeholder='Enter search value' style='width:200px'>");
            client.println(" <button id='sqlite-search-btn' onclick='searchSQLite()'>Search</button>");
            client.println("</div>");
            client.println("<div id='sqlite-result' style='margin-top:15px'></div>");
          } else {
            client.println("<p style='color:#ff9800;font-weight:bold'>SQLite Database not available</p>");
            client.println("<p>Please download the SQLite database first to enable advanced search.</p>");
          }
          client.println("</div>");

          // Card 6: CSV Database Download
          client.println("<div class='card'><h2>CSV Database Download</h2>");
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

          // Show update status for CSV
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
            client.print(" style='background:#4CAF50'");
          }
          client.println(">Download Database</button>");
          if (SD.exists(destFile)) {
            client.println(" <button onclick=\"if(confirm('Delete CSV database file?')) location.href='/delete'\" style=\"background:#dc3545\">Delete Database</button>");
          }
          client.println("<div id='progress-container' class='progress-container'>");
          client.println("<div class='status'>Status: <span id='status-text'>Starting...</span></div>");
          client.println("<div class='progress-bar'><div id='progress-fill' class='progress-fill'><span id='progress-text'>0%</span></div></div>");
          client.println("<p id='bytes'>0 / 0 MB</p>");
          client.println("</div>");
          client.println("</div>");

          // Card 7: Full SQLite Database Download
          client.println("<div class='card'><h2>SQLite Database Download</h2>");
          client.print("<p>Local file exists: ");
          client.println(SD.exists(destFile_db) ? "YES" : "NO");
          client.println("</p>");
          if (SD.exists(destFile_db)) {
            File dbFile = SD.open(destFile_db);
            if (dbFile) {
              client.print("<p>Local file size: ");
              client.print(dbFile.size());
              client.println(" bytes</p>");
              dbFile.close();
            }
          }

          // Show update status for SQLite DB
          if (remoteFileSize_db > 0) {
            client.print("<p>Remote file size: ");
            client.print(remoteFileSize_db);
            client.println(" bytes</p>");

            if (updateAvailable_db) {
              client.println("<p style='color:#ff9800;font-weight:bold'>Update available!</p>");
            } else {
              client.println("<p style='color:#4CAF50;font-weight:bold'>Database is up to date</p>");
            }
          }

          client.print("<button id='download-btn-db' onclick='startDownloadDB()'");
          if (updateAvailable_db) {
            client.print(" style='background:#4CAF50'");
          }
          client.println(">Download Database</button>");
          if (SD.exists(destFile_db)) {
            client.println(" <button onclick=\"if(confirm('Delete SQLite database file?')) location.href='/delete_db'\" style=\"background:#dc3545\">Delete Database</button>");
          }
          client.println("<div id='progress-container-db' class='progress-container'>");
          client.println("<div class='status'>Status: <span id='status-text-db'>Starting...</span></div>");
          client.println("<div class='progress-bar'><div id='progress-fill-db' class='progress-fill'><span id='progress-text-db'>0%</span></div></div>");
          client.println("<p id='bytes-db'>0 / 0 MB</p>");
          client.println("</div>");
          client.println("</div>");

          // Card 8: Custom File/Directory Delete
          client.println("<div class='card'><h2>Manual File/Directory Delete</h2>");
          client.println("<p>Delete specific files or directories from the SD card:</p>");
          client.println("<div style='margin:10px 0'>");
          client.println("<input type='text' id='delete-path' placeholder='/path/to/file or /path/to/directory' style='width:300px;padding:8px;border:1px solid #ddd;border-radius:3px'>");
          client.println(" <button id='delete-custom-btn' onclick='deleteCustomPath()' style='background:#dc3545'>Delete</button>");
          client.println("</div>");
          client.println("<div id='delete-result' style='margin-top:10px'></div>");
          client.println("<p style='color:#ff9800;font-size:12px;margin-top:10px'>");
          client.println("<strong>Warning:</strong> This will permanently delete the file or directory and all its contents. Use with caution!<br>");
          client.println("<strong>Examples:</strong> /test.txt, /.Trashes, /database/old_backup.csv<br>");
          client.println("<strong>Note:</strong> Wildcards (* or ?) are not supported. Specify exact file or directory paths only.");
          client.println("</p>");
          client.println("</div>");

          client.println("</body></html>");
        }
        else if (request.indexOf("GET /status_db") >= 0) {
          // Return JSON status for SQLite DB download AJAX polling
          String json = "{";
          json += "\"active\":";
          json += downloadActive_db ? "true" : "false";
          json += ",\"progress\":";
          json += String(downloadProgress_db);
          json += ",\"bytesWritten\":";
          json += String(downloadBytesWritten_db);
          json += ",\"bytesTotal\":";
          json += String(downloadBytesTotal_db);
          json += ",\"status\":\"";
          json += downloadStatus_db;
          json += "\"}";

          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println("Connection: close");
          client.print("Content-Length: ");
          client.println(json.length());
          client.println();
          client.println(json);
        }
        else if (request.indexOf("GET /status") >= 0) {
          // Return JSON status for CSV download AJAX polling
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
        else if (request.indexOf("GET /download_db") >= 0) {
          // Trigger SQLite DB download
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("Connection: close");
          client.println();
          client.println("OK");

          downloadRequested_db = true;
        }
        else if (request.indexOf("GET /download") >= 0) {
          // Trigger CSV download
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("Connection: close");
          client.println();
          client.println("OK");

          downloadRequested = true;
        }
        else if (request.indexOf("GET /delete_custom?path=") >= 0) {
          // Custom path delete API
          String path = "";
          
          // Extract path parameter
          int pathPos = request.indexOf("path=");
          if (pathPos != -1) {
            pathPos += 5;
            int endPos = request.indexOf(' ', pathPos);
            if (endPos == -1) endPos = request.indexOf('&', pathPos);
            if (endPos == -1) endPos = request.indexOf('\r', pathPos);
            path = request.substring(pathPos, endPos);
            
            // URL decode the path (basic decoding for common characters)
            path.replace("%2F", "/");
            path.replace("%2f", "/");
            path.replace("%20", " ");
            path.replace("+", " ");
            path.replace("%2E", ".");
            path.replace("%2e", ".");
            path.replace("%2A", "*");
            path.replace("%2a", "*");
            path.replace("%3F", "?");
            path.replace("%3f", "?");
            path.replace("%5B", "[");
            path.replace("%5b", "[");
            path.replace("%5D", "]");
            path.replace("%5d", "]");
          }

          Serial.print("Custom delete request for path: ");
          Serial.println(path);

          String jsonResponse;
          
          // Validate path
          if (path.length() == 0) {
            jsonResponse = "{\"success\":false,\"message\":\"No path specified\"}";
          } else if (!path.startsWith("/")) {
            jsonResponse = "{\"success\":false,\"message\":\"Path must start with /\"}";
          } else if (path == "/" || path == "/database") {
            jsonResponse = "{\"success\":false,\"message\":\"Cannot delete root or database directory\"}";
          } else if (path.indexOf("*") >= 0 || path.indexOf("?") >= 0) {
            jsonResponse = "{\"success\":false,\"message\":\"Wildcards (* or ?) are not supported. Please specify exact path.\"}";
          } else if (!SD.exists(path.c_str())) {
            jsonResponse = "{\"success\":false,\"message\":\"Path does not exist: " + path + "\"}";
          } else {
            // Perform deletion
            deleteRecursive(path.c_str());
            
            // Verify deletion
            if (!SD.exists(path.c_str())) {
              jsonResponse = "{\"success\":true,\"message\":\"Successfully deleted: " + path + "\"}";
              Serial.println("Deletion successful");
            } else {
              jsonResponse = "{\"success\":false,\"message\":\"Failed to delete path\"}";
              Serial.println("Deletion failed");
            }
          }

          // Send response
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println("Connection: close");
          client.println("Access-Control-Allow-Origin: *");
          client.print("Content-Length: ");
          client.println(jsonResponse.length());
          client.println();
          client.println(jsonResponse);
        }
        else if (request.indexOf("GET /delete_db") >= 0) {
          // Delete SQLite database file
          if (SD.exists(destFile_db)) {
            if (SD.remove(destFile_db)) {
              Serial.println("SQLite database file deleted via web interface");
            } else {
              Serial.println("Failed to delete SQLite database file");
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
        else if (request.indexOf("GET /delete") >= 0) {
          // Delete CSV database file
          if (SD.exists(destFile)) {
            if (SD.remove(destFile)) {
              Serial.println("CSV database file deleted via web interface");
            } else {
              Serial.println("Failed to delete CSV database file");
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
        else if (request.indexOf("GET /api/sqlite/search") >= 0) {
          // SQLite database search API
          String field = "radio_id";
          String value = "";

          // Extract field parameter
          int fieldPos = request.indexOf("field=");
          if (fieldPos != -1) {
            fieldPos += 6;
            int endPos = request.indexOf('&', fieldPos);
            if (endPos == -1) endPos = request.indexOf(' ', fieldPos);
            if (endPos == -1) endPos = request.indexOf('\r', fieldPos);
            field = request.substring(fieldPos, endPos);
          }

          // Extract value parameter
          int valuePos = request.indexOf("value=");
          if (valuePos != -1) {
            valuePos += 6;
            int endPos = request.indexOf('&', valuePos);
            if (endPos == -1) endPos = request.indexOf(' ', valuePos);
            if (endPos == -1) endPos = request.indexOf('\r', valuePos);
            value = request.substring(valuePos, endPos);
            // URL decode the value (basic: replace %20 with space)
            value.replace("%20", " ");
            value.replace("+", " ");
          }

          Serial.print("SQLite API search - field: ");
          Serial.print(field);
          Serial.print(", value: ");
          Serial.println(value);

          // Perform the search
          performSQLiteSearch(field.c_str(), value.c_str());

          // Send response
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println("Connection: close");
          client.println("Access-Control-Allow-Origin: *");
          client.print("Content-Length: ");
          client.println(sqliteSearchResult.length());
          client.println();
          client.println(sqliteSearchResult);
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
          checkForUpdate_db();
          return;
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

// FreeRTOS blinking task for DB download
void blinkTask_db(void* param) {
  while (downloadActive_db) {
    digitalWrite(DL_LED_PIN, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);  // Faster blink for DB
    digitalWrite(DL_LED_PIN, LOW);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  digitalWrite(DL_LED_PIN, LOW);
  vTaskDelete(NULL);
}

void performDownload_db() {
  Serial.println("=== SQLite DB Download requested ===");

  downloadProgress_db = 0;
  downloadBytesTotal_db = 0;
  downloadBytesWritten_db = 0;
  downloadStatus_db = "Connecting...";

  HTTPClient http;
  http.begin(fileURL_db);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    downloadBytesTotal_db = http.getSize();
    Serial.print("DB Content-Length: ");
    Serial.print((unsigned long)downloadBytesTotal_db);
    Serial.println(" bytes");

    // Delete old file if exists
    if (SD.exists(destFile_db)) {
      SD.remove(destFile_db);
      Serial.println("Removed old SQLite DB file");
    }

    File outFile = SD.open(destFile_db, FILE_WRITE);
    if (!outFile) {
      Serial.println("FAILED: Couldn't create DB file on SD!");
      http.end();
      downloadStatus_db = "ERROR: File open failed";
      downloadRequested_db = false;
      return;
    }
    Serial.println("DB File opened successfully!");

    // Start blinking LED
    downloadActive_db = true;
    TaskHandle_t blinkTaskHandle_db = NULL;
    xTaskCreatePinnedToCore(blinkTask_db, "BlinkTaskDB", 1024, NULL, 1, &blinkTaskHandle_db, 1);

    downloadStatus_db = "Downloading...";
    unsigned long downloadStart = millis();

    // Manual download with progress tracking
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[2048];
    unsigned long totalWritten = 0;
    int lastPercent = -1;

    Serial.println("Starting DB download...");

    while (http.connected() && (totalWritten < downloadBytesTotal_db || downloadBytesTotal_db == -1)) {
      // Check for incoming web requests
      WiFiClient statusClient = server.available();
      if (statusClient) {
        handleWebClient(statusClient);
      }

      size_t available = stream->available();

      if (available) {
        int bytesToRead = min((int)available, (int)sizeof(buffer));
        if (downloadBytesTotal_db > 0) {
          bytesToRead = min(bytesToRead, (int)(downloadBytesTotal_db - totalWritten));
        }

        int bytesRead = stream->readBytes(buffer, bytesToRead);

        if (bytesRead > 0) {
          size_t bytesWritten = outFile.write(buffer, bytesRead);
          totalWritten += bytesWritten;
          downloadBytesWritten_db = totalWritten;

          int percent = 0;
          if (downloadBytesTotal_db > 0) {
            percent = (int)((totalWritten * 100UL) / downloadBytesTotal_db);
          }
          downloadProgress_db = percent;

          if (percent != lastPercent && percent % 10 == 0) {
            Serial.printf("DB Progress: %d%% (%lu / %lu bytes)\n",
                         percent, totalWritten, downloadBytesTotal_db);
            lastPercent = percent;
          }
        }
        yield();
      } else {
        delay(1);
      }

      if (downloadBytesTotal_db > 0 && totalWritten >= downloadBytesTotal_db) {
        break;
      }
    }

    unsigned long downloadDuration = millis() - downloadStart;

    // Stop blinking LED
    downloadActive_db = false;
    delay(200);  // Wait for blink task to finish

    outFile.close();

    downloadBytesWritten_db = totalWritten;
    downloadProgress_db = 100;
    downloadStatus_db = "Download complete!";

    Serial.printf("SUCCESS! Downloaded DB %lu bytes in %lu ms\n", totalWritten, downloadDuration);

    // Verify file
    if (SD.exists(destFile_db)) {
      File verifyFile = SD.open(destFile_db);
      if (verifyFile) {
        size_t fileSize = verifyFile.size();
        Serial.print("Verified DB file size on SD: ");
        Serial.print(fileSize);
        Serial.println(" bytes");
        verifyFile.close();
      }
    }
  } else {
    Serial.print("HTTP GET failed for DB: ");
    Serial.println(http.errorToString(httpCode));
    downloadStatus_db = "ERROR: HTTP " + String(httpCode);
  }
  http.end();
  downloadRequested_db = false;
}

// SQLite callback function for search results - accumulates multiple rows
static int sqliteCallback(void *data, int argc, char **argv, char **azColName) {
  String* result = (String*)data;

  // Check if this is the first result
  if (result->length() == 0) {
    *result = "{\"results\":[";
  } else {
    // Add comma before next result
    result->remove(result->length() - 2, 2);  // Remove "]}"
    *result += ",";
  }

  // Add this row
  *result += "{";
  for (int i = 0; i < argc; i++) {
    String colName = String(azColName[i]);
    colName.toLowerCase();

    if (i > 0) *result += ",";

    // Handle numeric vs string fields
    if (colName == "radio_id" || colName == "id") {
      *result += "\"" + colName + "\":" + (argv[i] ? argv[i] : "null");
    } else {
      *result += "\"" + colName + "\":";
      if (argv[i]) {
        *result += "\"" + String(argv[i]) + "\"";
      } else {
        *result += "null";
      }
    }
  }
  *result += "}]}";  // Close object AND array

  return 0;
}

// Perform SQLite database search
void performSQLiteSearch(const char* searchField, const char* searchValue) {
  Serial.println("=== Starting SQLite search ===");
  Serial.print("Searching ");
  Serial.print(searchField);
  Serial.print(" for: ");
  Serial.println(searchValue);

  sqliteSearchResult = "";
  sqliteSearchActive = true;

  if (!SD.exists(destFile_db)) {
    Serial.println("ERROR: SQLite database file not found");
    sqliteSearchResult = "{\"error\":\"SQLite database not found\"}";
    sqliteSearchActive = false;
    return;
  }

  // Open database
  String dbPath = "/sd" + String(destFile_db);
  int rc = sqlite3_open(dbPath.c_str(), &db);

  if (rc != SQLITE_OK) {
    Serial.print("Can't open database: ");
    Serial.println(sqlite3_errmsg(db));
    sqliteSearchResult = "{\"error\":\"Failed to open database\"}";
    sqlite3_close(db);
    db = NULL;
    sqliteSearchActive = false;
    return;
  }

  Serial.println("Database opened successfully");

  // Ensure indexes exist (creates them if missing, no-op if they exist)
  // This handles older databases that don't have indexes
  Serial.println("Index RadioId");
  sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_radioid_radio_id ON radioid (RADIO_ID);", NULL, NULL, NULL);
  // Serial.println("Index Callsign");
  // sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_radioid_callsign ON radioid (CALLSIGN COLLATE NOCASE);", NULL, NULL, NULL);
  // Serial.println("Index First Name");
  // sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_radioid_first_name ON radioid (FIRST_NAME COLLATE NOCASE);", NULL, NULL, NULL);
  // Serial.println("Index City");
  // sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_radioid_city ON radioid (CITY COLLATE NOCASE);", NULL, NULL, NULL);
  // Serial.println("Index State");
  // sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_radioid_state ON radioid (STATE COLLATE NOCASE);", NULL, NULL, NULL);
  // Serial.println("Index Country");
  // sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_radioid_country ON radioid (COUNTRY COLLATE NOCASE);", NULL, NULL, NULL);

  // Build query based on search field
  // Schema: RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY
  String sql;
  if (strcmp(searchField, "radio_id") == 0) {
    // Support wildcard search with * (convert to SQL %)
    String searchStr = String(searchValue);
    searchStr.replace("*", "%");
    if (searchStr.indexOf('%') >= 0) {
      // Wildcard search
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE RADIO_ID LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 10000;";
    } else {
      // Exact match (faster)
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE RADIO_ID = " + searchStr + " LIMIT 1;";
    }
  } else if (strcmp(searchField, "callsign") == 0) {
    // Support wildcard search with * (convert to SQL %)
    String searchStr = String(searchValue);
    searchStr.replace("*", "%");
    if (searchStr.indexOf('%') >= 0) {
      // Wildcard search
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE CALLSIGN LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 10000;";
    } else {
      // Exact match (faster)
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE CALLSIGN = '" + searchStr + "' COLLATE NOCASE LIMIT 10000;";
    }
  } else if (strcmp(searchField, "name") == 0) {
    // Support wildcard search with * (convert to SQL %)
    String searchStr = String(searchValue);
    searchStr.replace("*", "%");
    if (searchStr.indexOf('%') >= 0) {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE FIRST_NAME LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 10000;";
    } else {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE FIRST_NAME = '" + searchStr + "' COLLATE NOCASE LIMIT 10000;";
    }
  } else if (strcmp(searchField, "city") == 0) {
    // Support wildcard search with * (convert to SQL %)
    String searchStr = String(searchValue);
    searchStr.replace("*", "%");
    if (searchStr.indexOf('%') >= 0) {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE CITY LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 10000;";
    } else {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE CITY = '" + searchStr + "' COLLATE NOCASE LIMIT 10000;";
    }
  } else if (strcmp(searchField, "state") == 0) {
    // Support wildcard search with * (convert to SQL %)
    String searchStr = String(searchValue);
    searchStr.replace("*", "%");
    if (searchStr.indexOf('%') >= 0) {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE STATE LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 10000;";
    } else {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE STATE = '" + searchStr + "' COLLATE NOCASE LIMIT 10000;";
    }
  } else if (strcmp(searchField, "country") == 0) {
    // Support wildcard search with * (convert to SQL %)
    String searchStr = String(searchValue);
    searchStr.replace("*", "%");
    if (searchStr.indexOf('%') >= 0) {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE COUNTRY LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 10000;";
    } else {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE COUNTRY = '" + searchStr + "' COLLATE NOCASE LIMIT 10000;";
    }
  } else {
    sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE RADIO_ID = " + String(searchValue) + " LIMIT 1;";
  }

  Serial.print("SQL: ");
  Serial.println(sql);

  char *errMsg = NULL;
  unsigned long startTime = millis();

  rc = sqlite3_exec(db, sql.c_str(), sqliteCallback, &sqliteSearchResult, &errMsg);

  unsigned long searchTime = millis() - startTime;
  Serial.print("Search completed in ");
  Serial.print(searchTime);
  Serial.println(" ms");

  if (rc != SQLITE_OK) {
    Serial.print("SQL error: ");
    Serial.println(errMsg);
    sqliteSearchResult = "{\"error\":\"" + String(errMsg) + "\"}";
    sqlite3_free(errMsg);
  }

  if (sqliteSearchResult.length() == 0) {
    sqliteSearchResult = "{\"results\":[]}";
  }

  sqlite3_close(db);
  db = NULL;
  sqliteSearchActive = false;

  Serial.print("Result: ");
  Serial.println(sqliteSearchResult);
  Serial.println("=== SQLite search complete ===");
}

void loop() {
  // Handle download requests from main loop
  if (downloadRequested) {
    performDownload();
  }

  // Handle SQLite DB download requests
  if (downloadRequested_db) {
    performDownload_db();
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