/*
 * Hardware Settings Page
 * GPIO pin and hardware peripheral settings (LED, OLED, SD Card).
 */

#ifndef WEB_SYSTEM_HARDWARE_H
#define WEB_SYSTEM_HARDWARE_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

// --- LED/Button ---
extern int buttonPin;
extern int ledPin;

// --- OLED ---
extern int i2cSclPin;
extern int i2cSdaPin;
extern int oledHeight;
extern int oledI2cAddress;
extern bool oledEnabled;
extern int oledWidth;

// --- SD Card ---
extern bool sdcardEnabled;
extern int sdCsPin;
extern int spiMisoPin;
extern int spiMosiPin;
extern int spiSclkPin;

String getSystemHardwarePageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Hardware Settings</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("system-hardware");

  html += "<div class='container'>";
  html += "<h1>Hardware Settings</h1>";
  html += "<p>Configure GPIO pins and hardware peripherals.</p>";

  html += "<div class='admin-grid'>";

  // Card 1: LED & Button Settings
  html += "<div class='card'>";
  html += "<h3>LED & Button Settings</h3>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>LED Pin:</span>";
  html += "<input type='text' id='led-pin' value='" + String(ledPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Button Pin:</span>";
  html += "<input type='text' id='button-pin' value='" + String(buttonPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "GPIO pin for built-in LED and OLED toggle button.";
  html += "</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveLedButtonSettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetLedButtonSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 2: OLED Settings
  html += "<div class='card'>";
  html += "<h3>OLED Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>OLED Enabled:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='oled-enable'" + String(oledEnabled ? " checked" : "") + ">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<details style='margin-top:10px;'>";
  html += "<summary style='cursor:pointer;color:#007bff;font-size:0.9em;'>Advanced Pin Configuration</summary>";
  html += "<div style='margin-top:8px;'>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>I2C SDA Pin:</span>";
  html += "<input type='text' id='i2c-sda' value='" + String(i2cSdaPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>I2C SCL Pin:</span>";
  html += "<input type='text' id='i2c-scl' value='" + String(i2cSclPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>I2C Address:</span>";
  {
    String hexAddr = "0x" + String(oledI2cAddress, HEX);
    hexAddr.toUpperCase();
    html += "<input type='text' id='oled-addr' value='" + (oledI2cAddress == 0x3C ? "0x3C" : (oledI2cAddress == 0x3D ? "0x3D" : hexAddr)) + "' style='width: 120px; padding-right: 8px;'>";
  }
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>OLED Width:</span>";
  html += "<input type='text' id='oled-width' value='" + String(oledWidth) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>OLED Height:</span>";
  html += "<input type='text' id='oled-height' value='" + String(oledHeight) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "</div>";
  html += "</details>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "Configure OLED display I2C pins, address, and resolution.";
  html += "</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveOledSettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetOledSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 3: SD Card Settings
  html += "<div class='card'>";
  html += "<h3>SD Card Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>SD Card Enabled:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='sdcard-enabled'" + String(sdcardEnabled ? " checked" : "") + ">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<details style='margin-top:10px;'>";
  html += "<summary style='cursor:pointer;color:#007bff;font-size:0.9em;'>Advanced Pin Configuration</summary>";
  html += "<div style='margin-top:8px;'>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>SPI MISO Pin:</span>";
  html += "<input type='text' id='spi-miso' value='" + String(spiMisoPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>SPI MOSI Pin:</span>";
  html += "<input type='text' id='spi-mosi' value='" + String(spiMosiPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>SPI SCLK Pin:</span>";
  html += "<input type='text' id='spi-sclk' value='" + String(spiSclkPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>SD CS Pin:</span>";
  html += "<input type='text' id='sd-cs' value='" + String(sdCsPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "</div>";
  html += "</details>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "Configure SD card SPI interface pins.";
  html += "</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveSdCardSettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetSdCardSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  // Save All & Reboot button
  html += "<div class='action-buttons-vertical' style='margin-top:20px;'>";
  html += "<button class='btn btn-success' onclick='saveAllHardwareSettings()'>Save All &amp; Reboot</button>";
  html += "</div>";
  html += "<div class='info' style='margin-top:20px'>";
  html += "<strong>Note:</strong> After changing settings, click Save All &amp; Reboot for all changes to take effect.";
  html += "</div>";

  // JavaScript functions
  html += "<script>";

  // Modal helpers
  html += "window.showModal = function(contentFn) {";
  html += "  var overlay = document.createElement('div');";
  html += "  overlay.className = 'modal-overlay';";
  html += "  var box = document.createElement('div');";
  html += "  box.className = 'modal-box';";
  html += "  contentFn(box, function() { document.body.removeChild(overlay); });";
  html += "  overlay.appendChild(box);";
  html += "  overlay.addEventListener('click', function(e) { if (e.target === overlay) document.body.removeChild(overlay); });";
  html += "  document.body.appendChild(overlay);";
  html += "  return overlay;";
  html += "};";
  html += "window.showAlert = function(msg) {";
  html += "  showModal(function(box, close) {";
  html += "    box.innerHTML = '<h4>' + msg + '</h4>';";
  html += "    var btns = document.createElement('div');";
  html += "    btns.className = 'modal-buttons';";
  html += "    var ok = document.createElement('button');";
  html += "    ok.textContent = 'OK';";
  html += "    ok.className = 'btn btn-primary';";
  html += "    ok.onclick = close;";
  html += "    btns.appendChild(ok);";
  html += "    box.appendChild(btns);";
  html += "  });";
  html += "};";
  html += "window.showConfirm = function(msg, onYes) {";
  html += "  showModal(function(box, close) {";
  html += "    box.innerHTML = '<h4>' + msg + '</h4>';";
  html += "    var btns = document.createElement('div');";
  html += "    btns.className = 'modal-buttons';";
  html += "    var yes = document.createElement('button');";
  html += "    yes.textContent = 'Yes';";
  html += "    yes.className = 'btn btn-success';";
  html += "    yes.onclick = function() { close(); onYes(); };";
  html += "    var no = document.createElement('button');";
  html += "    no.textContent = 'Cancel';";
  html += "    no.className = 'btn btn-danger';";
  html += "    no.onclick = close;";
  html += "    btns.appendChild(yes);";
  html += "    btns.appendChild(no);";
  html += "    box.appendChild(btns);";
  html += "  });";
  html += "};";

  // LED/Button Settings
  html += "function saveLedButtonSettings() {";
  html += "  var ledPin = document.getElementById('led-pin').value;";
  html += "  var buttonPin = document.getElementById('button-pin').value;";
  html += "  if (ledPin < 0 || ledPin > 48) { showAlert('LED pin must be 0-48'); return; }";
  html += "  if (buttonPin < 0 || buttonPin > 48) { showAlert('Button pin must be 0-48'); return; }";
  html += "  showConfirm('Save LED/Button settings and reboot?', function() {";
  html += "    fetch('/api/save-led-button-settings', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'ledpin=' + ledPin + '&buttonpin=' + buttonPin";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";
  html += "function resetLedButtonSettings() {";
  html += "  showConfirm('Reset LED/Button settings to default and reboot?', function() {";
  html += "    fetch('/api/reset-led-button-settings', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  // OLED Settings
  html += "function saveOledSettings() {";
  html += "  var oledEnabled = document.getElementById('oled-enable').checked ? '1' : '0';";
  html += "  var i2cSda = document.getElementById('i2c-sda').value;";
  html += "  var i2cScl = document.getElementById('i2c-scl').value;";
  html += "  var oledAddr = document.getElementById('oled-addr').value;";
  html += "  var oledWidth = document.getElementById('oled-width').value;";
  html += "  var oledHeight = document.getElementById('oled-height').value;";
  html += "  if (i2cSda < 0 || i2cSda > 48) { showAlert('I2C SDA pin must be 0-48'); return; }";
  html += "  if (i2cScl < 0 || i2cScl > 48) { showAlert('I2C SCL pin must be 0-48'); return; }";
  html += "  var addrUpper = oledAddr.toUpperCase(); if (addrUpper != '0X3C' && addrUpper != '0X3D') { showAlert('OLED I2C address must be 0x3C or 0x3D'); return; }";
  html += "  if (oledWidth < 64 || oledWidth > 256) { showAlert('OLED width must be 64-256'); return; }";
  html += "  if (oledHeight < 32 || oledHeight > 128) { showAlert('OLED height must be 32-128'); return; }";
  html += "  showConfirm('Save OLED settings and reboot?', function() {";
  html += "    fetch('/api/save-oled-settings', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'oledenabled=' + oledEnabled + '&i2csda=' + i2cSda + '&i2cscl=' + i2cScl + '&oledaddr=' + oledAddr + '&oledwidth=' + oledWidth + '&oledheight=' + oledHeight";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";
  html += "function resetOledSettings() {";
  html += "  showConfirm('Reset OLED settings to default and reboot?', function() {";
  html += "    fetch('/api/reset-oled-settings', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  // SD Card Settings
  html += "function saveSdCardSettings() {";
  html += "  var sdEnabled = document.getElementById('sdcard-enabled').checked ? '1' : '0';";
  html += "  var spiMiso = document.getElementById('spi-miso').value;";
  html += "  var spiMosi = document.getElementById('spi-mosi').value;";
  html += "  var spiSclk = document.getElementById('spi-sclk').value;";
  html += "  var sdCs = document.getElementById('sd-cs').value;";
  html += "  showConfirm('Save SD Card settings and reboot?', function() {";
  html += "    fetch('/api/save-sdcard-settings', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'sdcard_en=' + sdEnabled + '&spi_miso=' + spiMiso + '&spi_mosi=' + spiMosi + '&spi_sclk=' + spiSclk + '&sd_cs=' + sdCs";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";
  html += "function resetSdCardSettings() {";
  html += "  showConfirm('Reset SD Card settings to default and reboot?', function() {";
  html += "    fetch('/api/reset-sdcard-settings', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  // Save All & Reboot
  html += "function saveAllHardwareSettings() {";
  html += "  showConfirm('Save all hardware settings and reboot?', function() {";
  html += "    var post = function(url, body) {";
  html += "      return fetch(url, { method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: body });";
  html += "    };";
  html += "    var lPin = document.getElementById('led-pin').value;";
  html += "    var bPin = document.getElementById('button-pin').value;";
  html += "    var oledEn = document.getElementById('oled-enable').checked ? '1' : '0';";
  html += "    var sda = document.getElementById('i2c-sda').value;";
  html += "    var scl = document.getElementById('i2c-scl').value;";
  html += "    var oAddr = document.getElementById('oled-addr').value;";
  html += "    var oW = document.getElementById('oled-width').value;";
  html += "    var oH = document.getElementById('oled-height').value;";
  html += "    var sdEn = document.getElementById('sdcard-enabled').checked ? '1' : '0';";
  html += "    var sMiso = document.getElementById('spi-miso').value;";
  html += "    var sMosi = document.getElementById('spi-mosi').value;";
  html += "    var sSclk = document.getElementById('spi-sclk').value;";
  html += "    var sCsPin = document.getElementById('sd-cs').value;";
  html += "    post('/api/save-led-button-settings', 'ledpin=' + lPin + '&buttonpin=' + bPin)";
  html += "    .then(function() { return post('/api/save-oled-settings', 'oledenabled=' + oledEn + '&i2csda=' + sda + '&i2cscl=' + scl + '&oledaddr=' + oAddr + '&oledwidth=' + oW + '&oledheight=' + oH); })";
  html += "    .then(function() { return post('/api/save-sdcard-settings', 'sdcard_en=' + sdEn + '&spi_miso=' + sMiso + '&spi_mosi=' + sMosi + '&spi_sclk=' + sSclk + '&sd_cs=' + sCsPin); })";
  html += "    .then(function() {";
  html += "      showAlert('All hardware settings saved.<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    }).catch(function(err) {";
  html += "      showAlert('Error saving settings: ' + err);";
  html += "    });";
  html += "  });";
  html += "}";

  html += "</script>";

  html += "</div>"; // Close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SYSTEM_HARDWARE_H
