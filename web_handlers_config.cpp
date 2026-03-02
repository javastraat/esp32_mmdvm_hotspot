/*
 * web_handlers_config.cpp - Configuration Export & Import Routes
 *
 * Reads and writes ALL keys from the 'mmdvm' NVS namespace directly via the
 * ESP-IDF iterator, so no maintenance is needed when new settings are added.
 *
 * Export format:  key:type=value   (one per line, lines starting with # are comments)
 * Supported types: u8, i8, u16, i16, u32, i32, u64, i64, str
 *
 * Routes:
 *   /api/export-config  - dump all NVS settings as key:type=value text (attachment download)
 *   /api/import-config  - parse key:type=value text and write directly to NVS
 */

#include "system/web_handlers_config.h"
#include "system/system_webserver.h"   // extern WebServer server
#include "system/system_logger.h"      // addLogMessage()

#include <vector>
#include <algorithm>

extern "C" {
#include <nvs_flash.h>
#include <nvs.h>
}

extern void loadSettings();

// ---------------------------------------------------------------------------
// generateConfigString() — dump all 'mmdvm' NVS keys as key:type=value text.
// Shared by the export-config HTTP route and the snapshot-save handler.
// ---------------------------------------------------------------------------
String generateConfigString()
{
    String config = "# ESP32 MMDVM Configuration Export\n";
    config += "# Generated: " + String(millis() / 1000) + "s uptime\n";
    config += "# Format: key:type=value  (types: u8 i8 u16 i16 u32 i32 u64 i64 str)\n\n";

    nvs_handle_t handle;
    if (nvs_open("mmdvm", NVS_READONLY, &handle) != ESP_OK) {
        config += "# ERROR: Could not open NVS namespace\n";
        return config;
    }

    // Collect all entries first, then sort by key name
    std::vector<String> lines;
    nvs_iterator_t it = NULL;
    esp_err_t err = nvs_entry_find("nvs", "mmdvm", NVS_TYPE_ANY, &it);

    while (err == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);

        String typeName;
        String value;
        bool skip = false;

        switch (info.type) {
            case NVS_TYPE_U8: {
                uint8_t v = 0; nvs_get_u8(handle, info.key, &v);
                typeName = "u8"; value = String(v); break;
            }
            case NVS_TYPE_I8: {
                int8_t v = 0; nvs_get_i8(handle, info.key, &v);
                typeName = "i8"; value = String(v); break;
            }
            case NVS_TYPE_U16: {
                uint16_t v = 0; nvs_get_u16(handle, info.key, &v);
                typeName = "u16"; value = String(v); break;
            }
            case NVS_TYPE_I16: {
                int16_t v = 0; nvs_get_i16(handle, info.key, &v);
                typeName = "i16"; value = String(v); break;
            }
            case NVS_TYPE_U32: {
                uint32_t v = 0; nvs_get_u32(handle, info.key, &v);
                typeName = "u32"; value = String(v); break;
            }
            case NVS_TYPE_I32: {
                int32_t v = 0; nvs_get_i32(handle, info.key, &v);
                typeName = "i32"; value = String(v); break;
            }
            case NVS_TYPE_U64: {
                uint64_t v = 0; nvs_get_u64(handle, info.key, &v);
                char buf[21]; snprintf(buf, sizeof(buf), "%llu", (unsigned long long)v);
                typeName = "u64"; value = String(buf); break;
            }
            case NVS_TYPE_I64: {
                int64_t v = 0; nvs_get_i64(handle, info.key, &v);
                char buf[22]; snprintf(buf, sizeof(buf), "%lld", (long long)v);
                typeName = "i64"; value = String(buf); break;
            }
            case NVS_TYPE_STR: {
                size_t len = 0;
                nvs_get_str(handle, info.key, NULL, &len);
                if (len > 0) {
                    char* buf = (char*)malloc(len);
                    if (buf) {
                        nvs_get_str(handle, info.key, buf, &len);
                        value = String(buf);
                        free(buf);
                    }
                }
                typeName = "str"; break;
            }
            default:
                skip = true; break;  // Skip BLOBs and unknown types
        }

        if (!skip) {
            lines.push_back(String(info.key) + ":" + typeName + "=" + value);
        }
        err = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);
    nvs_close(handle);

    std::sort(lines.begin(), lines.end(), [](const String& a, const String& b) {
        return a.compareTo(b) < 0;
    });

    for (const auto& line : lines) config += line + "\n";
    config += "\n# Total: " + String(lines.size()) + " keys\n";
    return config;
}

// ---------------------------------------------------------------------------
// applyConfigString() — parse key:type=value and write directly to NVS.
// Returns the number of keys written. Calls loadSettings() to sync globals.
// Shared by the import-config HTTP route and the snapshot-load handler.
// ---------------------------------------------------------------------------
int applyConfigString(const String& body)
{
    nvs_handle_t handle;
    if (nvs_open("mmdvm", NVS_READWRITE, &handle) != ESP_OK) return 0;

    int imported = 0;
    int lineStart = 0;

    while (lineStart < (int)body.length()) {
        int lineEnd = body.indexOf('\n', lineStart);
        if (lineEnd == -1) lineEnd = body.length();

        String line = body.substring(lineStart, lineEnd);
        line.trim();
        lineStart = lineEnd + 1;

        if (line.length() == 0 || line.startsWith("#")) continue;

        int eq = line.indexOf('=');
        if (eq <= 0) continue;

        String keyType = line.substring(0, eq);
        String val = line.substring(eq + 1);
        keyType.trim();

        // Expect key:type — skip lines without a type annotation
        int colon = keyType.indexOf(':');
        if (colon <= 0) continue;

        String key = keyType.substring(0, colon);
        String typeName = keyType.substring(colon + 1);

        if (key.length() == 0 || key.length() > 15) continue;  // NVS key max 15 chars

        esp_err_t err = ESP_FAIL;
        if      (typeName == "u8")  err = nvs_set_u8 (handle, key.c_str(), (uint8_t) val.toInt());
        else if (typeName == "i8")  err = nvs_set_i8 (handle, key.c_str(), (int8_t)  val.toInt());
        else if (typeName == "u16") err = nvs_set_u16(handle, key.c_str(), (uint16_t)val.toInt());
        else if (typeName == "i16") err = nvs_set_i16(handle, key.c_str(), (int16_t) val.toInt());
        else if (typeName == "u32") err = nvs_set_u32(handle, key.c_str(), (uint32_t)strtoul(val.c_str(), NULL, 10));
        else if (typeName == "i32") err = nvs_set_i32(handle, key.c_str(), (int32_t) strtol (val.c_str(), NULL, 10));
        else if (typeName == "u64") err = nvs_set_u64(handle, key.c_str(), (uint64_t)strtoull(val.c_str(), NULL, 10));
        else if (typeName == "i64") err = nvs_set_i64(handle, key.c_str(), (int64_t) strtoll (val.c_str(), NULL, 10));
        else if (typeName == "str") err = nvs_set_str(handle, key.c_str(), val.c_str());
        else continue;  // Unknown type — skip

        if (err == ESP_OK) imported++;
    }

    nvs_commit(handle);
    nvs_close(handle);

    loadSettings();  // Refresh all runtime globals from the updated NVS

    return imported;
}

// ---------------------------------------------------------------------------
// registerConfigRoutes() — thin HTTP wrappers that delegate to the helpers.
// ---------------------------------------------------------------------------
void registerConfigRoutes()
{
    server.on("/api/export-config", HTTP_GET, []() {
        String config = generateConfigString();
        server.sendHeader("Content-Disposition", "attachment; filename=mmdvm-config.txt");
        server.send(200, "text/plain", config);
    });

    server.on("/api/import-config", HTTP_POST, []() {
        String body = server.arg("plain");
        if (body.length() == 0) {
            server.send(400, "text/plain", "ERROR: No configuration data received");
            return;
        }
        int imported = applyConfigString(body);
        addLogMessage("[System] Configuration imported: " + String(imported) + " settings applied");
        server.send(200, "text/plain", "Configuration imported: " + String(imported) + " settings applied");
    });
}
