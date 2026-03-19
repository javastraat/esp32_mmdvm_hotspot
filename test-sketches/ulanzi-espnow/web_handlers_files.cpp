// web_handlers_files.cpp — API handlers for LittleFS browser, serial log,
// file upload/download, and the LaMetric icon proxy.
#include "web_handlers_files.h"
#include "globals.h"
#include "serial_log.h"
#include <LittleFS.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "web/files.h"
#include "web/serial.h"

// Upload state — persists across the two upload callbacks
static File   _uploadFile;
static String _uploadedName;
static String _uploadDir;
static bool   _uploadOk;

void registerFileHandlers() {

  webServer.on("/files", HTTP_GET, []() {
    webServer.send_P(200, "text/html", PAGE_FILES);
  });

  webServer.on("/serial", HTTP_GET, []() {
    webServer.send_P(200, "text/html", PAGE_SERIAL);
  });

  webServer.on("/api/serial/log", HTTP_GET, []() {
    uint32_t cursor = webServer.hasArg("cursor")
      ? (uint32_t)webServer.arg("cursor").toInt() : 0;
    uint32_t newCursor;
    String data = serialLogQuery(cursor, &newCursor);
    String resp = "{\"data\":\"" + data + "\",\"cursor\":" + String(newCursor) + "}";
    webServer.send(200, "application/json", resp);
  });

  webServer.on("/api/serial/clear", HTTP_POST, []() {
    serialLogClear();
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/fs", HTTP_GET, []() {
    if (!fsAvailable) {
      webServer.send(503, "application/json", "{\"error\":\"fs unavailable\"}");
      return;
    }
    char buf[80];
    snprintf(buf, sizeof(buf), "{\"total\":%u,\"used\":%u,\"available\":%u}",
      LittleFS.totalBytes(), LittleFS.usedBytes(),
      LittleFS.totalBytes() - LittleFS.usedBytes());
    webServer.send(200, "application/json", buf);
  });

  webServer.on("/api/files", HTTP_GET, []() {
    if (!fsAvailable) {
      webServer.send(503, "application/json", "[]");
      return;
    }
    String json = "[";
    bool first = true;
    std::function<void(const String&)> listDir = [&](const String& dirPath) {
      File dir = LittleFS.open(dirPath);
      if (!dir) return;
      File f = dir.openNextFile();
      while (f) {
        if (f.isDirectory()) {
          listDir(String("/") + f.name());
        } else {
          if (!first) json += ",";
          json += "{\"name\":\"/";
          json += f.name();
          json += "\",\"size\":";
          json += f.size();
          json += "}";
          first = false;
        }
        f = dir.openNextFile();
      }
    };
    listDir("/");
    json += "]";
    webServer.send(200, "application/json", json);
  });

  webServer.on("/api/files/delete", HTTP_POST, []() {
    if (!fsAvailable) {
      webServer.send(503, "application/json", "{\"ok\":false,\"error\":\"fs unavailable\"}");
      return;
    }
    String name = webServer.arg("name");
    if (name.length() == 0) {
      webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"missing name\"}");
      return;
    }
    if (name[0] != '/') name = "/" + name;
    bool ok = LittleFS.remove(name);
    webServer.send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"not found\"}");
  });

  webServer.on("/api/files/upload", HTTP_POST,
    []() {
      if (_uploadOk) {
        char resp[120];
        snprintf(resp, sizeof(resp), "{\"ok\":true,\"name\":\"%s\"}", _uploadedName.c_str());
        webServer.send(200, "application/json", resp);
      } else {
        webServer.send(500, "application/json", "{\"ok\":false,\"error\":\"write failed\"}");
      }
    },
    []() {
      HTTPUpload& up = webServer.upload();
      if (up.status == UPLOAD_FILE_START) {
        _uploadDir = webServer.arg("dir");
        if (_uploadDir.length() == 0 || _uploadDir[0] != '/') _uploadDir = "/icons";
        if (!LittleFS.exists(_uploadDir)) LittleFS.mkdir(_uploadDir);
        _uploadedName = (_uploadDir == "/") ? "/" + up.filename : _uploadDir + "/" + up.filename;
        _uploadOk     = false;
        LittleFS.remove(_uploadedName);
        _uploadFile = LittleFS.open(_uploadedName, "w");
        LOG("[FS] Upload start: %s\n", _uploadedName.c_str());
      } else if (up.status == UPLOAD_FILE_WRITE) {
        if (_uploadFile)
          _uploadOk = (_uploadFile.write(up.buf, up.currentSize) == up.currentSize);
      } else if (up.status == UPLOAD_FILE_END) {
        if (_uploadFile) {
          _uploadFile.close();
          LOG("[FS] Upload done: %s  %u bytes\n",
            _uploadedName.c_str(), up.totalSize);
        }
      }
    }
  );

  // ── Filesystem browser API ──────────────────────────────────────────────────

  webServer.on("/api/fs/ls", HTTP_GET, []() {
    if (!fsAvailable) { webServer.send(503, "application/json", "{\"entries\":[]}"); return; }
    String path = webServer.arg("path");
    if (!path.length()) path = "/";
    File dir = LittleFS.open(path);
    if (!dir || !dir.isDirectory()) {
      webServer.send(200, "application/json", "{\"entries\":[]}"); return;
    }
    String json = "{\"entries\":[";
    bool first = true;
    File f = dir.openNextFile();
    while (f) {
      String raw = f.name();
      int sl = raw.lastIndexOf('/');
      String bname = (sl >= 0) ? raw.substring(sl + 1) : raw;
      String fullPath = (path == "/") ? "/" + bname : path + "/" + bname;
      if (!first) json += ",";
      json += "{\"name\":\"" + bname + "\",\"path\":\"" + fullPath + "\",\"isDir\":";
      json += f.isDirectory() ? "true" : "false";
      json += ",\"size\":" + String((unsigned long)f.size()) + "}";
      first = false;
      f = dir.openNextFile();
    }
    json += "]}";
    webServer.send(200, "application/json", json);
  });

  webServer.on("/api/fs/download", HTTP_GET, []() {
    if (!fsAvailable) { webServer.send(503, "text/plain", "FS unavailable"); return; }
    String path = webServer.arg("path");
    if (!path.length() || path[0] != '/') { webServer.send(400, "text/plain", "Bad path"); return; }
    File f = LittleFS.open(path, "r");
    if (!f || f.isDirectory()) { if (f) f.close(); webServer.send(404, "text/plain", "Not found"); return; }
    int sl = path.lastIndexOf('/');
    String fname = (sl >= 0) ? path.substring(sl + 1) : path;
    webServer.sendHeader("Content-Disposition", "attachment; filename=\"" + fname + "\"");
    webServer.streamFile(f, "application/octet-stream");
    f.close();
  });

  webServer.on("/api/fs/delete", HTTP_POST, []() {
    if (!fsAvailable) { webServer.send(503, "text/plain", "FS unavailable"); return; }
    String path = webServer.arg("path");
    if (!path.length() || path[0] != '/') { webServer.send(400, "text/plain", "Bad path"); return; }
    File f = LittleFS.open(path);
    bool isDir = f && f.isDirectory();
    if (f) f.close();
    bool ok = isDir ? LittleFS.rmdir(path) : LittleFS.remove(path);
    if (!ok && isDir) { webServer.send(500, "text/plain", "Cannot delete — folder may not be empty"); return; }
    if (!ok)          { webServer.send(500, "text/plain", "Delete failed"); return; }
    webServer.send(200, "text/plain", "Deleted: " + path);
  });

  webServer.on("/api/fs/mkdir", HTTP_POST, []() {
    if (!fsAvailable) { webServer.send(503, "text/plain", "FS unavailable"); return; }
    String path = webServer.arg("path");
    if (!path.length() || path[0] != '/') { webServer.send(400, "text/plain", "Bad path"); return; }
    bool ok = LittleFS.mkdir(path);
    webServer.send(ok ? 200 : 500, "text/plain", ok ? "Created: " + path : "Failed");
  });

  webServer.on("/api/fs/rename", HTTP_POST, []() {
    if (!fsAvailable) { webServer.send(503, "text/plain", "FS unavailable"); return; }
    String from = webServer.arg("from");
    String to   = webServer.arg("to");
    if (!from.length() || from[0] != '/' || !to.length() || to[0] != '/') {
      webServer.send(400, "text/plain", "Bad path"); return;
    }
    bool ok = LittleFS.rename(from, to);
    webServer.send(ok ? 200 : 500, "text/plain", ok ? "Renamed" : "Rename failed");
  });

  // Proxy LaMetric icon to browser — browser does PNG→JPEG conversion via canvas,
  // then uploads the result via /api/files/upload.
  webServer.on("/api/icons/proxy", HTTP_GET, []() {
    String id = webServer.arg("id");
    if (id.length() == 0 || id.length() > 6) {
      webServer.send(400, "text/plain", "invalid id"); return;
    }
    for (size_t i = 0; i < id.length(); i++) {
      if (!isDigit(id[i])) { webServer.send(400, "text/plain", "invalid id"); return; }
    }
    WiFiClientSecure tlsClient;
    tlsClient.setInsecure();
    HTTPClient http;
    http.begin(tlsClient, "https://developer.lametric.com/content/apps/icon_thumbs/" + id);
    http.setTimeout(8000);
    int code = http.GET();
    if (code != 200) {
      http.end();
      webServer.send(404, "text/plain", "not found");
      return;
    }
    const int MAX_DL = 8192;
    uint8_t* buf = (uint8_t*)malloc(MAX_DL);
    if (!buf) { http.end(); webServer.send(500, "text/plain", "no memory"); return; }

    WiFiClient* stream = http.getStreamPtr();
    int total = 0;
    uint32_t deadline = millis() + 8000;
    while (http.connected() && millis() < deadline && total < MAX_DL) {
      int avail = stream->available();
      if (avail > 0) {
        int n = stream->readBytes(buf + total, min(avail, MAX_DL - total));
        total += n;
        deadline = millis() + 4000;
      } else { delay(10); }
      int cl = http.getSize();
      if (cl > 0 && total >= cl) break;
    }
    http.end();

    // Detect type from magic bytes
    const char* ct;
    if (total >= 3 && buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F')
      ct = "image/gif";
    else if (total >= 2 && buf[0] == 0xFF && buf[1] == 0xD8)
      ct = "image/jpeg";
    else if (total >= 4 && buf[0] == 0x89 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G')
      ct = "image/png";
    else
      ct = "application/octet-stream";

    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send_P(200, ct, (const char*)buf, total);
    free(buf);
  });
}
