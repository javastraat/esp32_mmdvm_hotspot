/*
 * web_handlers_nvs.cpp - NVS Preferences Viewer & Repair Routes
 *
 * Extracted from web_handlers_admin.cpp.
 * All routes registered here use the global `server` object (extern WebServer server).
 *
 * Routes:
 *   /api/show-prefs          - read all known NVS keys and return formatted HTML table
 *   /api/show-prefs-raw      - enumerate all NVS keys via ESP-IDF iterator (any namespace)
 *   /api/repair-prefs        - add missing NVS keys with config.h defaults, then reboot
 *   /api/list-nvs-namespaces - list all NVS namespaces (JSON)
 *   /api/prefs-reset         - erase the mmdvm NVS namespace
 */

#include "system/web_handlers_nvs.h"
#include "system/system_webserver.h"   // extern WebServer server
#include "system/system_logger.h"      // addLogMessage()
#include "include/config.h"            // DEFAULT_* compile-time constants
#include <Preferences.h>

extern "C"
{
#include <nvs_flash.h>
#include <nvs.h>
}

void registerNvsRoutes()
{
  // Show Preferences - reads all NVS keys and returns formatted HTML
  server.on("/api/show-prefs", HTTP_GET, []()
            {
    extern Preferences preferences;
    preferences.begin("mmdvm", true);  // Read-only mode

    int keyCount = 0;

    // Helper: renders a table row with a masked password field + lock/reveal toggle.
    // Inline onclick is used intentionally — browsers do NOT execute <script> tags
    // injected via innerHTML (which is how showPrefsInline() inserts this content),
    // but onclick attributes on individual elements are registered and fire correctly.
    // The WireGuard private key is excluded — use a plain row for that one.
    auto secretRow = [](const String& key, const String& val) -> String {
      String row = "<tr><td>" + key + "</td><td class='val'>";
      if (val.isEmpty()) { row += "(not set)"; }
      else {
        // Escape & and " so they don't break the data-val attribute
        String safe = val; safe.replace("&", "&amp;"); safe.replace("\"", "&quot;");
        // Span shows bullet dots; onclick swaps textContent with the stored value.
        // \u2022 is the JS unicode escape for • — interpreted by JS, not the HTML parser.
        row += "<span data-val=\"" + safe + "\" data-shown=\"0\" class=\"secret-dots\">"
               "&#x2022;&#x2022;&#x2022;&#x2022;&#x2022;&#x2022;&#x2022;&#x2022;</span>"
               " <button"
               " onclick=\"var s=this.previousElementSibling;"
               "if(s.dataset.shown==='1'){s.textContent='\\u2022\\u2022\\u2022\\u2022\\u2022\\u2022\\u2022\\u2022';"
               "s.dataset.shown='0';this.innerHTML='&#x1F512;';}else{s.textContent=s.dataset.val;"
               "s.dataset.shown='1';this.innerHTML='&#x1F513;';}\""
               " class=\"lock-btn\" title=\"Show / hide\">&#x1F512;</button>";
      }
      return row + "</td></tr>";
    };

    String html =
      "<style>"
        ".secret-dots{color:#888;letter-spacing:3px;font-family:monospace;}"
        ".lock-btn{background:none;border:none;cursor:pointer;font-size:1.1em;"
          "padding:0 4px;vertical-align:middle;}"
      "</style>"
      "<h2>NVS Preferences (mmdvm namespace) - {{KEY_COUNT}} keys</h2>";

    html += "<h3>Mode Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>initialized</td><td class='val'>" + String(preferences.getBool("initialized", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>mode_dmr</td><td class='val'>" + String(preferences.getBool("mode_dmr", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>mode_dstar</td><td class='val'>" + String(preferences.getBool("mode_dstar", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>mode_ysf</td><td class='val'>" + String(preferences.getBool("mode_ysf", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>mode_p25</td><td class='val'>" + String(preferences.getBool("mode_p25", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>mode_nxdn</td><td class='val'>" + String(preferences.getBool("mode_nxdn", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>mode_pocsag</td><td class='val'>" + String(preferences.getBool("mode_pocsag", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>dapnet_en</td><td class='val'>" + String(preferences.getBool("dapnet_en", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>cwid_en</td><td class='val'>" + String(preferences.getBool("cwid_en", false) ? "true" : "false") + "</td></tr>";
    html += "</table>";

    html += "<h3>CW ID Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>cwid_int</td><td class='val'>" + String(preferences.getUChar("cwid_int", CWID_INTERVAL_MIN)) + "</td></tr>";
    html += "</table>";

    html += "<h3>DAPNET / POCSAG Network Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>pocsag_freq</td><td class='val'>" + String(preferences.getUInt("pocsag_freq", POCSAG_FREQUENCY)) + "</td></tr>";
    html += "<tr><td>dapnet_server</td><td class='val'>" + preferences.getString("dapnet_server", "") + "</td></tr>";
    html += "<tr><td>dapnet_port</td><td class='val'>" + String(preferences.getUShort("dapnet_port", 43434)) + "</td></tr>";
    html += "<tr><td>dapnet_cs</td><td class='val'>" + preferences.getString("dapnet_cs", "") + "</td></tr>";
    html += secretRow("dapnet_key", preferences.getString("dapnet_key", ""));
    html += "<tr><td>dapnet_ric</td><td class='val'>" + String(preferences.getUInt("dapnet_ric", 0)) + "</td></tr>";
    html += "<tr><td>pocsag_wlist</td><td class='val'>" + preferences.getString("pocsag_wlist", "") + "</td></tr>";
    html += "<tr><td>pocsag_blist</td><td class='val'>" + preferences.getString("pocsag_blist", "") + "</td></tr>";
    html += "</table>";

    html += "<h3>Station Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>callsign</td><td class='val'>" + preferences.getString("callsign", "") + "</td></tr>";
    html += "<tr><td>dmr_id</td><td class='val'>" + String(preferences.getUInt("dmr_id", 0)) + "</td></tr>";
    html += "<tr><td>dmr_ssid</td><td class='val'>" + String(preferences.getUChar("dmr_ssid", 0)) + "</td></tr>";
    html += "</table>";

    html += "<h3>RF Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>dmr_rx_freq</td><td class='val'>" + String(preferences.getUInt("dmr_rx_freq", 0)) + "</td></tr>";
    html += "<tr><td>dmr_tx_freq</td><td class='val'>" + String(preferences.getUInt("dmr_tx_freq", 0)) + "</td></tr>";
    html += "<tr><td>dmr_color_code</td><td class='val'>" + String(preferences.getUChar("dmr_color_code", 0)) + "</td></tr>";
    html += "<tr><td>dmr_rf_power</td><td class='val'>" + String(preferences.getUChar("dmr_rf_power", 0)) + "</td></tr>";
    html += "</table>";

    html += "<h3>DMR Server Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>dmr_server</td><td class='val'>" + preferences.getString("dmr_server", "") + "</td></tr>";
    html += "<tr><td>dmr_port</td><td class='val'>" + String(preferences.getUShort("dmr_port", 0)) + "</td></tr>";
    html += "<tr><td>dmr_lport</td><td class='val'>" + String(preferences.getUShort("dmr_lport", 0)) + "</td></tr>";
    html += secretRow("dmr_pass", preferences.getString("dmr_pass", ""));
    html += "<tr><td>dmr_hist_size</td><td class='val'>" + String(preferences.getUShort("dmr_hist_size", 0)) + "</td></tr>";
    html += "<tr><td>dmr_act_tout</td><td class='val'>" + String(preferences.getUShort("dmr_act_tout", 0)) + "</td></tr>";
    html += "<tr><td>dmr_usr_cache</td><td class='val'>" + String(preferences.getUShort("dmr_usr_cache", 0)) + "</td></tr>";
    html += "<tr><td>dmr_cs_cache</td><td class='val'>" + String(preferences.getUShort("dmr_cs_cache", 0)) + "</td></tr>";
    html += "<tr><td>dmr_api_tout</td><td class='val'>" + String(preferences.getUShort("dmr_api_tout", 0)) + "</td></tr>";
    html += "</table>";

    html += "<h3>Hotspot Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>hs_callsign</td><td class='val'>" + preferences.getString("hs_callsign", "") + "</td></tr>";
    html += "<tr><td>hs_suffix</td><td class='val'>" + preferences.getString("hs_suffix", "") + "</td></tr>";
    html += "<tr><td>hs_latitude</td><td class='val'>" + preferences.getString("hs_latitude", "0.0") + "</td></tr>";
    html += "<tr><td>hs_longitude</td><td class='val'>" + preferences.getString("hs_longitude", "0.0") + "</td></tr>";
    html += "<tr><td>hs_height</td><td class='val'>" + String(preferences.getInt("hs_height", 0)) + "</td></tr>";
    html += "<tr><td>hs_location</td><td class='val'>" + preferences.getString("hs_location", "") + "</td></tr>";
    html += "<tr><td>hs_desc</td><td class='val'>" + preferences.getString("hs_desc", "") + "</td></tr>";
    html += "<tr><td>hs_url</td><td class='val'>" + preferences.getString("hs_url", "") + "</td></tr>";
    html += "</table>";

    html += "<h3>DMR API Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>dmr_api_url</td><td class='val'>" + preferences.getString("dmr_api_url", "") + "</td></tr>";
    html += "<tr><td>qrz_lookup_url</td><td class='val'>" + preferences.getString("qrz_lookup_url", "") + "</td></tr>";
    html += "</table>";

    html += "<h3>WiFi Station Settings (6 slots)</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>wifi_s0_lbl</td><td class='val'>" + preferences.getString("wifi_s0_lbl", "") + "</td></tr>";
    html += "<tr><td>wifi_ssid</td><td class='val'>" + preferences.getString("wifi_ssid", "") + "</td></tr>";
    html += secretRow("wifi_pass",  preferences.getString("wifi_pass",  ""));
    html += "<tr><td>wifi_s1_lbl</td><td class='val'>" + preferences.getString("wifi_s1_lbl", "") + "</td></tr>";
    html += "<tr><td>wifi_ssid1</td><td class='val'>" + preferences.getString("wifi_ssid1", "") + "</td></tr>";
    html += secretRow("wifi_pass1", preferences.getString("wifi_pass1", ""));
    html += "<tr><td>wifi_s2_lbl</td><td class='val'>" + preferences.getString("wifi_s2_lbl", "") + "</td></tr>";
    html += "<tr><td>wifi_ssid2</td><td class='val'>" + preferences.getString("wifi_ssid2", "") + "</td></tr>";
    html += secretRow("wifi_pass2", preferences.getString("wifi_pass2", ""));
    html += "<tr><td>wifi_s3_lbl</td><td class='val'>" + preferences.getString("wifi_s3_lbl", "") + "</td></tr>";
    html += "<tr><td>wifi_ssid3</td><td class='val'>" + preferences.getString("wifi_ssid3", "") + "</td></tr>";
    html += secretRow("wifi_pass3", preferences.getString("wifi_pass3", ""));
    html += "<tr><td>wifi_s4_lbl</td><td class='val'>" + preferences.getString("wifi_s4_lbl", "") + "</td></tr>";
    html += "<tr><td>wifi_ssid4</td><td class='val'>" + preferences.getString("wifi_ssid4", "") + "</td></tr>";
    html += secretRow("wifi_pass4", preferences.getString("wifi_pass4", ""));
    html += "<tr><td>wifi_s5_lbl</td><td class='val'>" + preferences.getString("wifi_s5_lbl", "") + "</td></tr>";
    html += "<tr><td>wifi_ssid5</td><td class='val'>" + preferences.getString("wifi_ssid5", "") + "</td></tr>";
    html += secretRow("wifi_pass5", preferences.getString("wifi_pass5", ""));
    html += "</table>";

    html += "<h3>WiFi AP Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>wifi_ap_ssid</td><td class='val'>" + preferences.getString("wifi_ap_ssid", "") + "</td></tr>";
    html += secretRow("wifi_ap_pass", preferences.getString("wifi_ap_pass", ""));
    html += "<tr><td>wifi_ap_ch</td><td class='val'>" + String(preferences.getUChar("wifi_ap_ch", 0)) + "</td></tr>";
    html += "<tr><td>wifi_max_ret</td><td class='val'>" + String(preferences.getUChar("wifi_max_ret", 0)) + "</td></tr>";
    html += "</table>";

    html += "<h3>Ethernet Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>eth_enabled</td><td class='val'>" + String(preferences.getBool("eth_enabled", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>eth_debug</td><td class='val'>" + String(preferences.getBool("eth_debug", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>eth_miso</td><td class='val'>" + String(preferences.getInt("eth_miso", 0)) + "</td></tr>";
    html += "<tr><td>eth_mosi</td><td class='val'>" + String(preferences.getInt("eth_mosi", 0)) + "</td></tr>";
    html += "<tr><td>eth_sclk</td><td class='val'>" + String(preferences.getInt("eth_sclk", 0)) + "</td></tr>";
    html += "<tr><td>eth_cs</td><td class='val'>" + String(preferences.getInt("eth_cs", 0)) + "</td></tr>";
    html += "<tr><td>eth_int</td><td class='val'>" + String(preferences.getInt("eth_int", 0)) + "</td></tr>";
    html += "<tr><td>eth_rst</td><td class='val'>" + String(preferences.getInt("eth_rst", 0)) + "</td></tr>";
    html += "<tr><td>eth_addr</td><td class='val'>" + String(preferences.getInt("eth_addr", 0)) + "</td></tr>";
    html += "<tr><td>eth_cto</td><td class='val'>" + String(preferences.getInt("eth_cto", 0)) + "</td></tr>";
    html += "</table>";

    html += "<h3>WireGuard VPN Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>wg_en</td><td class='val'>" + String(preferences.getBool("wg_en", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>wg_local_ip</td><td class='val'>" + preferences.getString("wg_local_ip", "") + "</td></tr>";
    html += "<tr><td>wg_priv_key</td><td class='val'>" + String(preferences.getString("wg_priv_key", "").isEmpty() ? "(not set)" : "&#x1F510; (set &mdash; never displayed)") + "</td></tr>";
    html += secretRow("wg_pub_key", preferences.getString("wg_pub_key", ""));
    html += "<tr><td>wg_endpoint</td><td class='val'>" + preferences.getString("wg_endpoint", "") + "</td></tr>";
    html += "<tr><td>wg_ep_port</td><td class='val'>" + String(preferences.getUShort("wg_ep_port", 51820)) + "</td></tr>";
    html += "<tr><td>wg_dns</td><td class='val'>" + preferences.getString("wg_dns", "") + "</td></tr>";
    html += "<tr><td>wg_allowed_ips</td><td class='val'>" + preferences.getString("wg_allowed_ips", "") + "</td></tr>";
    html += "</table>";

    html += "<h3>DNS Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>dns_fb_en</td><td class='val'>" + String(preferences.getBool("dns_fb_en", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>dns_fb_ip</td><td class='val'>" + preferences.getString("dns_fb_ip", "") + "</td></tr>";
    html += "</table>";

    html += "<h3>mDNS Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>mdns_en</td><td class='val'>" + String(preferences.getBool("mdns_en", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>mdns_host</td><td class='val'>" + preferences.getString("mdns_host", "") + "</td></tr>";
    html += "</table>";

    html += "<h3>NTP Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>ntp_en</td><td class='val'>" + String(preferences.getBool("ntp_en", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>ntp_srv</td><td class='val'>" + preferences.getString("ntp_srv", "") + "</td></tr>";
    html += "<tr><td>ntp_gmt</td><td class='val'>" + String(preferences.getInt("ntp_gmt", 0)) + "</td></tr>";
    html += "<tr><td>ntp_dst</td><td class='val'>" + String(preferences.getInt("ntp_dst", 0)) + "</td></tr>";
    html += "<tr><td>ntp_sync</td><td class='val'>" + String(preferences.getUInt("ntp_sync", 0)) + "</td></tr>";
    html += "</table>";

    html += "<h3>MQTT Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>mqtt_en</td><td class='val'>" + String(preferences.getBool("mqtt_en", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>mqtt_broker</td><td class='val'>" + preferences.getString("mqtt_broker", "") + "</td></tr>";
    html += "<tr><td>mqtt_port</td><td class='val'>" + String(preferences.getUShort("mqtt_port", 0)) + "</td></tr>";
    html += "<tr><td>mqtt_user</td><td class='val'>" + preferences.getString("mqtt_user", "") + "</td></tr>";
    html += secretRow("mqtt_pass", preferences.getString("mqtt_pass", ""));
    html += "<tr><td>mqtt_status</td><td class='val'>" + preferences.getString("mqtt_status", "") + "</td></tr>";
    html += "<tr><td>mqtt_logs</td><td class='val'>" + preferences.getString("mqtt_logs", "") + "</td></tr>";
    html += "<tr><td>mqtt_hw</td><td class='val'>" + preferences.getString("mqtt_hw", "") + "</td></tr>";
    html += "<tr><td>mq_log_task</td><td class='val'>" + preferences.getString("mq_log_task", "") + "</td></tr>";
    html += "<tr><td>mqtt_oled_task</td><td class='val'>" + preferences.getString("mqtt_oled_task", "") + "</td></tr>";
    html += "<tr><td>mqtt_led_task</td><td class='val'>" + preferences.getString("mqtt_led_task", "") + "</td></tr>";
    html += "<tr><td>mqtt_wifi_task</td><td class='val'>" + preferences.getString("mqtt_wifi_task", "") + "</td></tr>";
    html += "<tr><td>mqtt_eth_task</td><td class='val'>" + preferences.getString("mqtt_eth_task", "") + "</td></tr>";
    html += "<tr><td>mq_ota_task</td><td class='val'>" + preferences.getString("mq_ota_task", "") + "</td></tr>";
    html += "<tr><td>mqtt_ntp_task</td><td class='val'>" + preferences.getString("mqtt_ntp_task", "") + "</td></tr>";
    html += "<tr><td>mq_mqttc_task</td><td class='val'>" + preferences.getString("mq_mqttc_task", "") + "</td></tr>";
    html += "<tr><td>mq_web_task</td><td class='val'>" + preferences.getString("mq_web_task", "") + "</td></tr>";
    html += "<tr><td>mq_sd_task</td><td class='val'>" + preferences.getString("mq_sd_task", "") + "</td></tr>";
    html += "<tr><td>mq_sensor_task</td><td class='val'>" + preferences.getString("mq_sensor_task", "") + "</td></tr>";
    html += "<tr><td>mqtt_wg_task</td><td class='val'>" + preferences.getString("mqtt_wg_task", "") + "</td></tr>";

    html += "<tr><td>mqtt_modem_task</td><td class='val'>" + preferences.getString("mqtt_modem_task", "") + "</td></tr>";
    html += "<tr><td>mqtt_dmr_task</td><td class='val'>" + preferences.getString("mqtt_dmr_task", "") + "</td></tr>";
    html += "<tr><td>mqtt_dstar_task</td><td class='val'>" + preferences.getString("mqtt_dstar_task", "") + "</td></tr>";
    html += "<tr><td>mqtt_ysf_task</td><td class='val'>" + preferences.getString("mqtt_ysf_task", "") + "</td></tr>";
    html += "<tr><td>mqtt_p25_task</td><td class='val'>" + preferences.getString("mqtt_p25_task", "") + "</td></tr>";
    html += "<tr><td>mqtt_nxdn_task</td><td class='val'>" + preferences.getString("mqtt_nxdn_task", "") + "</td></tr>";
    html += "<tr><td>mq_pocsag_task</td><td class='val'>" + preferences.getString("mq_pocsag_task", "") + "</td></tr>";
    html += "<tr><td>mq_dapnet_task</td><td class='val'>" + preferences.getString("mq_dapnet_task", "") + "</td></tr>";

    html += "<tr><td>mqtt_sub</td><td class='val'>" + preferences.getString("mqtt_sub", "") + "</td></tr>";
    html += "<tr><td>mqtt_hw_int</td><td class='val'>" + String(preferences.getUShort("mqtt_hw_int", 0)) + "</td></tr>";
    html += "<tr><td>mqtt_hw_log</td><td class='val'>" + String(preferences.getBool("mqtt_hw_log", false) ? "true" : "false") + "</td></tr>";
    html += "</table>";

    html += "<h3>Web Server Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>web_en</td><td class='val'>" + String(preferences.getBool("web_en", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>web_user</td><td class='val'>" + preferences.getString("web_user", "") + "</td></tr>";
    html += secretRow("web_pass", preferences.getString("web_pass", ""));
    html += "<tr><td>web_port</td><td class='val'>" + String(preferences.getUShort("web_port", 0)) + "</td></tr>";
    html += "</table>";

    html += "<h3>Hardware Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>led_pin</td><td class='val'>" + String(preferences.getInt("led_pin", 0)) + "</td></tr>";
    html += "<tr><td>button_pin</td><td class='val'>" + String(preferences.getInt("button_pin", 0)) + "</td></tr>";
    html += "<tr><td>oled_en</td><td class='val'>" + String(preferences.getBool("oled_en", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>i2c_sda</td><td class='val'>" + String(preferences.getInt("i2c_sda", 0)) + "</td></tr>";
    html += "<tr><td>i2c_scl</td><td class='val'>" + String(preferences.getInt("i2c_scl", 0)) + "</td></tr>";
    html += "<tr><td>oled_addr</td><td class='val'>" + String(preferences.getInt("oled_addr", 0)) + "</td></tr>";
    html += "<tr><td>oled_w</td><td class='val'>" + String(preferences.getInt("oled_w", 0)) + "</td></tr>";
    html += "<tr><td>oled_h</td><td class='val'>" + String(preferences.getInt("oled_h", 0)) + "</td></tr>";
    html += "</table>";

    html += "<h3>SD Card Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>sdcard_en</td><td class='val'>" + String(preferences.getBool("sdcard_en", false) ? "true" : "false") + "</td></tr>";
    html += "<tr><td>spi_miso</td><td class='val'>" + String(preferences.getInt("spi_miso", 0)) + "</td></tr>";
    html += "<tr><td>spi_mosi</td><td class='val'>" + String(preferences.getInt("spi_mosi", 0)) + "</td></tr>";
    html += "<tr><td>spi_sclk</td><td class='val'>" + String(preferences.getInt("spi_sclk", 0)) + "</td></tr>";
    html += "<tr><td>sd_cs</td><td class='val'>" + String(preferences.getInt("sd_cs", 0)) + "</td></tr>";
    html += "</table>";

    html += "<h3>MMDVM Serial Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>mmdvm_rx</td><td class='val'>" + String(preferences.getInt("mmdvm_rx", 0)) + "</td></tr>";
    html += "<tr><td>mmdvm_tx</td><td class='val'>" + String(preferences.getInt("mmdvm_tx", 0)) + "</td></tr>";
    html += "<tr><td>mmdvm_boot</td><td class='val'>" + String(preferences.getInt("mmdvm_boot", 0)) + "</td></tr>";
    html += "<tr><td>mmdvm_rst</td><td class='val'>" + String(preferences.getInt("mmdvm_rst", 0)) + "</td></tr>";
    html += "<tr><td>mmdvm_wakeup</td><td class='val'>" + String(preferences.getInt("mmdvm_wakeup", 0)) + "</td></tr>";
    html += "<tr><td>mmdvm_baud</td><td class='val'>" + String(preferences.getInt("mmdvm_baud", 0)) + "</td></tr>";
    html += "</table>";

    html += "<h3>MMDVM RF Calibration</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>mmdvm_txdly</td><td class='val'>" + String(preferences.getInt("mmdvm_txdly", 0)) + "</td></tr>";
    html += "<tr><td>mmdvm_rxlvl</td><td class='val'>" + String(preferences.getInt("mmdvm_rxlvl", 0)) + "</td></tr>";
    html += "<tr><td>mmdvm_txlvl</td><td class='val'>" + String(preferences.getInt("mmdvm_txlvl", 0)) + "</td></tr>";
    html += "</table>";

    html += "<h3>ArduinoOTA Settings</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>ota_en</td><td class='val'>" + String(preferences.getBool("ota_en", false) ? "true" : "false") + "</td></tr>";
    html += secretRow("ota_pass", preferences.getString("ota_pass", ""));
    html += "<tr><td>ota_port</td><td class='val'>" + String(preferences.getInt("ota_port", 0)) + "</td></tr>";
    html += "</table>";

    html += "<h3>Firmware Info</h3><table><tr><th>Key</th><th>Value</th></tr>";
    html += "<tr><td>fw_app0</td><td class='val'>" + preferences.getString("fw_app0", "(not set)") + "</td></tr>";
    html += "</table>";

    preferences.end();

    // Count keys by counting table rows
    int idx = 0;
    while ((idx = html.indexOf("<tr><td>", idx)) != -1) { keyCount++; idx++; }
    html.replace("{{KEY_COUNT}}", String(keyCount));

    addLogMessage("[Admin] Show preferences requested");
    server.send(200, "text/html", html); });


  // Show Preferences RAW - enumerate all NVS keys using ESP-IDF iterator
  server.on("/api/show-prefs-raw", HTTP_GET, []() {
    String ns = server.hasArg("namespace") ? server.arg("namespace") : String("mmdvm");
    int keyCount = 0;
    String html =
      "<style>"
        ".secret-dots{color:#888;letter-spacing:3px;font-family:monospace;}"
        ".lock-btn{background:none;border:none;cursor:pointer;font-size:1.1em;"
          "padding:0 4px;vertical-align:middle;}"
      "</style>"
      "<h2>NVS Preferences RAW (" + ns + " namespace) - {{KEY_COUNT}} keys</h2>";
    html += "<table><tr><th>#</th><th>Key</th><th>Type</th><th>Value</th></tr>";

    nvs_handle_t handle;
    esp_err_t err = nvs_open(ns.c_str(), NVS_READONLY, &handle);
    if (err == ESP_OK) {
      nvs_iterator_t it = NULL;
      err = nvs_entry_find("nvs", ns.c_str(), NVS_TYPE_ANY, &it);
      while (err == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        keyCount++;

        String typeName;
        String value;
        switch (info.type) {
          case NVS_TYPE_U8: {
            uint8_t v; nvs_get_u8(handle, info.key, &v);
            typeName = "u8"; value = String(v);
            break;
          }
          case NVS_TYPE_I8: {
            int8_t v; nvs_get_i8(handle, info.key, &v);
            typeName = "i8"; value = String(v);
            break;
          }
          case NVS_TYPE_U16: {
            uint16_t v; nvs_get_u16(handle, info.key, &v);
            typeName = "u16"; value = String(v);
            break;
          }
          case NVS_TYPE_I16: {
            int16_t v; nvs_get_i16(handle, info.key, &v);
            typeName = "i16"; value = String(v);
            break;
          }
          case NVS_TYPE_U32: {
            uint32_t v; nvs_get_u32(handle, info.key, &v);
            typeName = "u32"; value = String(v);
            break;
          }
          case NVS_TYPE_I32: {
            int32_t v; nvs_get_i32(handle, info.key, &v);
            typeName = "i32"; value = String(v);
            break;
          }
          case NVS_TYPE_U64: {
            uint64_t v; nvs_get_u64(handle, info.key, &v);
            typeName = "u64"; value = String((uint32_t)v);
            break;
          }
          case NVS_TYPE_I64: {
            int64_t v; nvs_get_i64(handle, info.key, &v);
            typeName = "i64"; value = String((int32_t)v);
            break;
          }
          case NVS_TYPE_STR: {
            size_t len = 0;
            nvs_get_str(handle, info.key, NULL, &len);
            if (len > 0) {
              char* buf = (char*)malloc(len);
              if (!buf) { server.send(500, "text/plain", "OOM"); return; }
              nvs_get_str(handle, info.key, buf, &len);
              value = String(buf);
              free(buf);
            }
            typeName = "str";
            break;
          }
          case NVS_TYPE_BLOB: {
            typeName = "blob"; value = "(binary data)";
            break;
          }
          default: {
            typeName = "?"; value = "unknown";
            break;
          }
        }
        // Mask sensitive credentials in the value column
        static const char* SENSITIVE_RAW[] = {
          "wifi_pass","wifi_pass1","wifi_pass2","wifi_pass3","wifi_pass4","wifi_pass5",
          "wifi_ap_pass","dmr_pass","dapnet_key","mqtt_pass","web_pass","ota_pass","wg_pub_key"
        };
        String cellContent;
        if (strcmp(info.key, "wg_priv_key") == 0) {
          cellContent = value.isEmpty() ? "(not set)" : "&#x1F510; (never displayed)";
        } else {
          bool isSensitive = false;
          for (const char* sk : SENSITIVE_RAW) { if (strcmp(info.key, sk) == 0) { isSensitive = true; break; } }
          if (isSensitive && !value.isEmpty()) {
            String safe = value; safe.replace("&", "&amp;"); safe.replace("\"", "&quot;");
            cellContent = "<span data-val=\"" + safe + "\" data-shown=\"0\" class=\"secret-dots\">"
                          "&#x2022;&#x2022;&#x2022;&#x2022;&#x2022;&#x2022;&#x2022;&#x2022;</span>"
                          " <button"
                          " onclick=\"var s=this.previousElementSibling;"
                          "if(s.dataset.shown==='1'){s.textContent='\\u2022\\u2022\\u2022\\u2022\\u2022\\u2022\\u2022\\u2022';"
                          "s.dataset.shown='0';this.innerHTML='&#x1F512;';}else{s.textContent=s.dataset.val;"
                          "s.dataset.shown='1';this.innerHTML='&#x1F513;';}\""
                          " class=\"lock-btn\">&#x1F512;</button>";
          } else {
            cellContent = value;
          }
        }
        html += "<tr><td>" + String(keyCount) + "</td><td>" + String(info.key) + "</td><td>" + typeName + "</td><td class='val'>" + cellContent + "</td></tr>";
        err = nvs_entry_next(&it);
      }
      nvs_release_iterator(it);
      nvs_close(handle);
    } else {
      html += "<tr><td colspan='4'>Error opening NVS namespace</td></tr>";
    }

    html += "</table>";
    html.replace("{{KEY_COUNT}}", String(keyCount));

    addLogMessage("[Admin] Show preferences RAW requested for namespace: " + ns);
    server.send(200, "text/html", html);
  });


  // Repair Preferences - checks all keys and adds missing ones with config.h defaults
  server.on("/api/repair-prefs", HTTP_POST, []()
            {
    extern Preferences preferences;
    preferences.begin("mmdvm", false);  // Read-write mode

    int repaired = 0;
    int total = 0;

    // Helper: check if key exists, if not write default
    // Bool keys
    struct BoolPref {
      const char* key;
      bool defaultVal;
    };
    BoolPref boolPrefs[] = {
      { "initialized", true },
      { "mode_dmr", DEFAULT_MODE_DMR },
      { "mode_dstar", DEFAULT_MODE_DSTAR },
      { "mode_ysf", DEFAULT_MODE_YSF },
      { "mode_p25", DEFAULT_MODE_P25 },
      { "mode_nxdn", DEFAULT_MODE_NXDN },
      { "mode_pocsag", DEFAULT_MODE_POCSAG },
      { "dapnet_en",   DAPNET_ENABLED },
      { "cwid_en",     CWID_ENABLED },
      { "eth_enabled", ETH_ENABLED },
      { "eth_debug", ETH_DEBUG },
      { "wg_en", WG_ENABLED },
      { "dns_fb_en", DNS_FALLBACK_ENABLED },
      { "mdns_en", ENABLE_MDNS },
      { "ntp_en", NTP_ENABLED },
      { "mqtt_en", MQTT_ENABLED },
      { "mqtt_hw_log", MQTT_HARDWARE_INFO_LOG },
      { "oled_en", OLED_ENABLED },
      { "sdcard_en", SDCARD_ENABLED },
      { "ota_en", ARDUINO_OTA_ENABLED },
      { "web_en", WEB_ENABLED }
    };
    for (auto& p : boolPrefs) {
      total++;
      if (!preferences.isKey(p.key)) {
        preferences.putBool(p.key, p.defaultVal);
        repaired++;
      }
    }

    // String keys
    struct StrPref {
      const char* key;
      const char* defaultVal;
    };
    StrPref strPrefs[] = {
      { "callsign", DMR_CALLSIGN },
      { "wifi_s0_lbl", WIFI_SLOT_LABEL },
      { "wifi_ssid", WIFI_SSID },
      { "wifi_pass", WIFI_PASSWORD },
      { "wifi_s1_lbl", WIFI_SLOT1_LABEL },
      { "wifi_ssid1", WIFI_SSID1 },
      { "wifi_pass1", WIFI_PASSWORD1 },
      { "wifi_s2_lbl", WIFI_SLOT2_LABEL },
      { "wifi_ssid2", WIFI_SSID2 },
      { "wifi_pass2", WIFI_PASSWORD2 },
      { "wifi_s3_lbl", WIFI_SLOT3_LABEL },
      { "wifi_ssid3", WIFI_SSID3 },
      { "wifi_pass3", WIFI_PASSWORD3 },
      { "wifi_s4_lbl", WIFI_SLOT4_LABEL },
      { "wifi_ssid4", WIFI_SSID4 },
      { "wifi_pass4", WIFI_PASSWORD4 },
      { "wifi_s5_lbl", WIFI_SLOT5_LABEL },
      { "wifi_ssid5", WIFI_SSID5 },
      { "wifi_pass5", WIFI_PASSWORD5 },
      { "wifi_ap_ssid", WIFI_AP_SSID },
      { "wifi_ap_pass", WIFI_AP_PASSWORD },
      { "dns_fb_ip", DNS_FALLBACK_IP },
      { "mdns_host", MDNS_HOSTNAME },
      { "ntp_srv", NTP_SERVER },
      { "mqtt_broker", MQTT_BROKER },
      { "mqtt_user", MQTT_USER },
      { "mqtt_pass", MQTT_PASSWORD },
      { "mqtt_status", MQTT_STATUS_TOPIC },
      { "mqtt_logs", MQTT_LOGS_TOPIC },
      { "mqtt_hw", MQTT_HARDWARE_TOPIC },
      { "mqtt_sub", MQTT_SUBSCRIBE_TOPIC },
      { "mq_log_task", MQTT_LOGGER_TASK_TOPIC },
      { "mqtt_oled_task", MQTT_OLED_TASK_TOPIC },
      { "mqtt_led_task", MQTT_LED_TASK_TOPIC },
      { "mqtt_wifi_task", MQTT_WIFI_TASK_TOPIC },
      { "mqtt_eth_task", MQTT_ETH_TASK_TOPIC },
      { "mq_ota_task", MQTT_ARDUINO_OTA_TASK_TOPIC },
      { "mqtt_ntp_task", MQTT_NTP_TASK_TOPIC },
      { "mq_mqttc_task", MQTT_MQTT_CLIENT_TASK_TOPIC },
      { "mq_web_task", MQTT_WEB_SERVER_TASK_TOPIC },
      { "mq_sd_task", MQTT_SD_CARD_TASK_TOPIC },
      { "mq_sensor_task", MQTT_SENSOR_TASK_TOPIC },
      { "mqtt_wg_task", MQTT_WG_TASK_TOPIC },
      { "mqtt_modem_task", MQTT_MODEM_TASK_TOPIC },
      { "mqtt_dmr_task", MQTT_DMR_TASK_TOPIC },
      { "mqtt_dstar_task", MQTT_DSTAR_TASK_TOPIC },
      { "mqtt_ysf_task", MQTT_YSF_TASK_TOPIC },
      { "mqtt_p25_task", MQTT_P25_TASK_TOPIC },
      { "mqtt_nxdn_task", MQTT_NXDN_TASK_TOPIC },
      { "mq_pocsag_task", MQTT_POCSAG_TASK_TOPIC },
      { "mq_dapnet_task", MQTT_DAPNET_TASK_TOPIC },
      { "dapnet_server", DAPNET_SERVER },
      { "dapnet_key",    DAPNET_AUTH_KEY },
      { "dapnet_cs",     DAPNET_NODE_CS },
      { "pocsag_wlist",  POCSAG_WHITELIST },
      { "pocsag_blist",  POCSAG_BLACKLIST },
      { "wg_local_ip", WG_LOCAL_IP },
      { "wg_priv_key", WG_PRIVATE_KEY },
      { "wg_pub_key", WG_PUBLIC_KEY },
      { "wg_endpoint", WG_ENDPOINT },
      { "wg_dns", WG_DNS },
      { "wg_allowed_ips", WG_ALLOWED_IPS },
      { "ota_pass", ARDUINO_OTA_PASSWORD },
      { "web_user", WEB_USERNAME },
      { "web_pass", WEB_PASSWORD },
      { "dmr_server", DMR_SERVER },
      { "dmr_pass", DMR_PASSWORD },
      { "hs_callsign", HOTSPOT_CALLSIGN },
      { "hs_suffix", HOTSPOT_SUFFIX },
      { "hs_latitude", HOTSPOT_LATITUDE },
      { "hs_longitude", HOTSPOT_LONGITUDE },
      { "hs_location", HOTSPOT_LOCATION },
      { "hs_desc", HOTSPOT_DESCRIPTION },
      { "hs_url", HOTSPOT_URL },
      { "dmr_api_url", DMR_API_URL },
      { "qrz_lookup_url", QRZ_LOOKUP_URL }
    };
    for (auto& p : strPrefs) {
      total++;
      if (!preferences.isKey(p.key)) {
        preferences.putString(p.key, p.defaultVal);
        repaired++;
      }
    }

    // UInt keys
    struct UIntPref {
      const char* key;
      uint32_t defaultVal;
    };
    UIntPref uintPrefs[] = {
      { "dmr_id", DMR_ID },
      { "dmr_rx_freq", DMR_RX_FREQ },
      { "dmr_tx_freq", DMR_TX_FREQ },
      { "ntp_sync", NTP_SYNC_INTERVAL_MS },
      { "pocsag_freq", POCSAG_FREQUENCY },
      { "dapnet_ric", 0 }
    };
    for (auto& p : uintPrefs) {
      total++;
      if (!preferences.isKey(p.key)) {
        preferences.putUInt(p.key, p.defaultVal);
        repaired++;
      }
    }

    // UChar keys
    struct UCharPref {
      const char* key;
      uint8_t defaultVal;
    };
    UCharPref ucharPrefs[] = {
      { "cwid_int",     CWID_INTERVAL_MIN },
      { "dmr_ssid", DMR_SSID },
      { "dmr_color_code", DMR_COLOR_CODE },
      { "dmr_rf_power", DMR_RF_POWER },
      { "wifi_ap_ch", WIFI_AP_CHANNEL },
      { "wifi_max_ret", WIFI_MAX_RETRIES }
    };
    for (auto& p : ucharPrefs) {
      total++;
      if (!preferences.isKey(p.key)) {
        preferences.putUChar(p.key, p.defaultVal);
        repaired++;
      }
    }

    // UShort keys
    struct UShortPref {
      const char* key;
      uint16_t defaultVal;
    };
    UShortPref ushortPrefs[] = {
      { "mqtt_port", MQTT_PORT }, { "mqtt_hw_int", MQTT_SEND_HARDWARE_INFO }, { "web_port", WEB_SERVER_PORT },
      { "dmr_port", DMR_PORT }, { "dmr_lport", DMR_LOCAL_PORT }, { "dapnet_port", DAPNET_PORT },
      { "dmr_hist_size", DMR_HISTORY_SIZE }, { "dmr_act_tout", DMR_ACTIVITY_TIMEOUT },
      { "dmr_usr_cache", DMR_USER_CACHE_SIZE }, { "dmr_cs_cache", DMR_CALLSIGN_CACHE_SIZE },
      { "dmr_api_tout", DMR_API_TIMEOUT },
      { "wg_ep_port", WG_ENDPOINT_PORT }
    };
    for (auto& p : ushortPrefs) {
      total++;
      if (!preferences.isKey(p.key)) {
        preferences.putUShort(p.key, p.defaultVal);
        repaired++;
      }
    }

    // Int keys
    struct IntPref {
      const char* key;
      int32_t defaultVal;
    };
    IntPref intPrefs[] = {
      { "ntp_gmt", NTP_GMT_OFFSET_SEC }, { "ntp_dst", NTP_DAYLIGHT_OFFSET_SEC }, { "led_pin", LED_PIN }, { "button_pin", BUTTON_PIN }, { "i2c_sda", I2C_SDA_PIN }, { "i2c_scl", I2C_SCL_PIN }, { "oled_addr", OLED_I2C_ADDRESS }, { "oled_w", OLED_WIDTH }, { "oled_h", OLED_HEIGHT }, { "spi_miso", SPI_MISO_PIN }, { "spi_mosi", SPI_MOSI_PIN }, { "spi_sclk", SPI_SCLK_PIN }, { "sd_cs", SD_CS_PIN }, { "eth_miso", ETH_MISO_PIN }, { "eth_mosi", ETH_MOSI_PIN }, { "eth_sclk", ETH_SCLK_PIN }, { "eth_cs", ETH_CS_PIN }, { "eth_int", ETH_INT_PIN }, { "eth_rst", ETH_RST_PIN }, { "eth_addr", ETH_ADDR }, { "eth_cto", ETH_CONNECT_TIMEOUT }, { "mmdvm_rx", MMDVM_RX_PIN }, { "mmdvm_tx", MMDVM_TX_PIN }, { "mmdvm_boot", MMDVM_BOOT_PIN }, { "mmdvm_rst", MMDVM_RESET_PIN }, { "mmdvm_wakeup", MMDVM_WAKEUP_PIN }, { "mmdvm_baud", MMDVM_SERIAL_BAUD }, { "mmdvm_txdly", MMDVM_TX_DELAY }, { "mmdvm_rxlvl", MMDVM_RX_LEVEL }, { "mmdvm_txlvl", MMDVM_TX_LEVEL }, { "ota_port", ARDUINO_OTA_PORT },
      { "hs_height", HOTSPOT_HEIGHT }
    };
    for (auto& p : intPrefs) {
      total++;
      if (!preferences.isKey(p.key)) {
        preferences.putInt(p.key, p.defaultVal);
        repaired++;
      }
    }


    preferences.end();

    String msg;
    if (repaired > 0) {
      msg = "Repaired: " + String(repaired) + " missing keys added (out of " + String(total) + " checked).\nSystem will reboot in 3 seconds.";
      addLogMessage("[Admin] Preferences repaired: " + String(repaired) + " keys added");
      server.send(200, "text/plain", msg);
      delay(3000);
      ESP.restart();
    } else {
      msg = "All " + String(total) + " preference keys present. No repair needed.";
      addLogMessage("[Admin] Preferences check: all " + String(total) + " keys OK");
      server.send(200, "text/plain", msg);
    } });

  // List all NVS namespaces (JSON)
  server.on("/api/list-nvs-namespaces", HTTP_GET, []() {
    std::vector<String> namespaces;
    nvs_iterator_t it = NULL;
    esp_err_t err = nvs_entry_find("nvs", NULL, NVS_TYPE_ANY, &it);
    while (err == ESP_OK) {
      nvs_entry_info_t info;
      nvs_entry_info(it, &info);
      String ns = String(info.namespace_name);
      bool found = false;
      for (auto &n : namespaces) { if (n == ns) { found = true; break; } }
      if (!found) namespaces.push_back(ns);
      err = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);
    String json = "{\"namespaces\": [";
    for (size_t i = 0; i < namespaces.size(); ++i) {
      json += '"' + namespaces[i] + '"';
      if (i < namespaces.size() - 1) json += ",";
    }
    json += "]}";
    server.send(200, "application/json", json);
  });

  // Erase all keys in the 'mmdvm' NVS namespace
  server.on("/api/prefs-reset", HTTP_POST, []()
            {
    nvs_handle_t nvsHandle;
    esp_err_t err = nvs_open("mmdvm", NVS_READWRITE, &nvsHandle);
    if (err == ESP_OK) {
      err = nvs_erase_all(nvsHandle);
      nvs_commit(nvsHandle);
      nvs_close(nvsHandle);
      if (err == ESP_OK) {
        addLogMessage("[Settings] All preferences erased from NVS (namespace: mmdvm)");
        server.send(200, "text/plain", "SUCCESS: All preferences erased from NVS (namespace: mmdvm)");
        return;
      }
    }
    addLogMessage("[Settings] Failed to erase NVS: " + String(err));
    server.send(500, "text/plain", "ERROR: Failed to erase NVS (code: " + String(err) + ")"); });
}
