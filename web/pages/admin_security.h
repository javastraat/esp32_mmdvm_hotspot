/*
 * admin_security.h - Admin Security Page
 * Web username and password management
 */

#ifndef WEB_PAGES_ADMIN_SECURITY_H
#define WEB_PAGES_ADMIN_SECURITY_H

#include <Arduino.h>
#include "../common/css.h"
#include "../common/navigation.h"
#include "../common/utils.h"

// Forward declarations of handler functions
void handleSaveUsername();
void handleSaveWebPassword();

// Forward declarations
extern String web_username;
extern String web_password;

void handleAdminSecurity() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Security Settings - " + dmr_callsign + "</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".admin-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 20px 0; }";
  html += "@media (max-width: 768px) { .admin-grid { grid-template-columns: 1fr; } }";
  html += ".btn { display: inline-block; padding: 12px 24px; margin: 10px 5px; border: none; border-radius: 6px; cursor: pointer; font-size: 14px; font-weight: bold; }";
  html += ".btn-success { background: #28a745; color: white; }";
  html += ".btn-success:hover { background: #218838; }";
  html += "</style></head><body>";
  html += getNavigation("admin");
  html += "<div class='container'>";
  html += "<h1>Security Settings</h1>";

  // Start admin grid container
  html += "<div class='admin-grid'>";

  // Web Username Card
  html += "<div class='card'>";
  html += "<h3>Web Username</h3>";
  html += "<p>Manage web interface username</p>";
  html += "<div style='background:var(--info-bg);padding:10px;border-radius:4px;margin-bottom:15px;'>";
  html += "<div style='display:flex;justify-content:space-between;align-items:center;'>";
  html += "<span><strong>Current Username:</strong></span>";
  html += "<span id='current-username-display'>" + web_username + "</span>";
  html += "</div>";
  html += "</div>";

  html += "<form id='username-form' onsubmit='saveUsername(event)'>";
  html += "<label>New Username:</label>";
  html += "<input type='text' id='new-username' placeholder='Enter new username' value='" + web_username + "' required style='width:100%;padding:8px;margin:5px 0;box-sizing:border-box;'>";
  html += "<p style='font-size:0.85em;color:#666;margin:5px 0;'>Username must be at least 3 characters</p>";
  html += "<button type='submit' class='btn btn-info' style='width:100%;margin-top:10px;'>Update Username</button>";
  html += "</form>";
  html += "</div>";

  // Web Password Card
  html += "<div class='card'>";
  html += "<h3>Web Password</h3>";
  html += "<p>Manage web interface password</p>";
  html += "<div style='background:var(--info-bg);padding:10px;border-radius:4px;margin-bottom:15px;'>";
  html += "<div style='display:flex;justify-content:space-between;align-items:center;'>";
  html += "<span><strong>Current Password:</strong></span>";
  html += "<div style='display:flex;align-items:center;gap:8px;'>";
  html += "<span id='current-password-display' style='font-family:monospace;'>********</span>";
  html += "<span onclick='toggleCurrentPassword()' style='cursor:pointer;font-size:18px;' title='Show/Hide Password'>&#128065;</span>";
  html += "<span id='current-password-real' style='display:none;font-family:monospace;'>" + web_password + "</span>";
  html += "</div>";
  html += "</div>";
  html += "</div>";

  html += "<form id='password-form' onsubmit='saveWebPassword(event)'>";
  html += "<label>New Password:</label>";
  html += "<div style='position:relative;'>";
  html += "<input type='password' id='new-password' placeholder='Enter new password' required style='width:100%;padding:8px;padding-right:40px;margin:5px 0;box-sizing:border-box;'>";
  html += "<span onclick='togglePasswordField(\"new-password\")' style='position:absolute;right:10px;top:50%;transform:translateY(-50%);cursor:pointer;font-size:18px;' title='Show/Hide'>&#128065;</span>";
  html += "</div>";
  html += "<label>Confirm Password:</label>";
  html += "<div style='position:relative;'>";
  html += "<input type='password' id='confirm-password' placeholder='Confirm new password' required style='width:100%;padding:8px;padding-right:40px;margin:5px 0;box-sizing:border-box;'>";
  html += "<span onclick='togglePasswordField(\"confirm-password\")' style='position:absolute;right:10px;top:50%;transform:translateY(-50%);cursor:pointer;font-size:18px;' title='Show/Hide'>&#128065;</span>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin:5px 0;'>Password must be at least 4 characters</p>";
  html += "<button type='submit' class='btn btn-success' style='width:100%;margin-top:10px;'>Change Password</button>";
  html += "</form>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  // JavaScript functions
  html += "<script>";
  html += "function toggleCurrentPassword() {";
  html += "  var masked = document.getElementById('current-password-display');";
  html += "  var real = document.getElementById('current-password-real');";
  html += "  if (masked.style.display === 'none') {";
  html += "    masked.style.display = 'inline';";
  html += "    real.style.display = 'none';";
  html += "  } else {";
  html += "    masked.style.display = 'none';";
  html += "    real.style.display = 'inline';";
  html += "  }";
  html += "}";
  html += "function togglePasswordField(fieldId) {";
  html += "  var field = document.getElementById(fieldId);";
  html += "  field.type = field.type === 'password' ? 'text' : 'password';";
  html += "}";
  html += "function saveUsername(event) {";
  html += "  event.preventDefault();";
  html += "  var newUsername = document.getElementById('new-username').value.trim();";
  html += "  if (newUsername.length < 3) {";
  html += "    alert('Username must be at least 3 characters long!');";
  html += "    return;";
  html += "  }";
  html += "  if (confirm('Are you sure you want to change the web username to \"' + newUsername + '\"? You will need to log in again with the new username.')) {";
  html += "    fetch('/save-username', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'username=' + encodeURIComponent(newUsername)}).then(response => response.text()).then(data => {";
  html += "      if (data.includes('SUCCESS')) {";
  html += "        alert('Username changed successfully! Please log in again with your new username.');";
  html += "        window.location.href = '/';";
  html += "      } else {";
  html += "        alert('Error: ' + data);";
  html += "      }";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function saveWebPassword(event) {";
  html += "  event.preventDefault();";
  html += "  var newPassword = document.getElementById('new-password').value;";
  html += "  var confirmPassword = document.getElementById('confirm-password').value;";
  html += "  if (newPassword !== confirmPassword) {";
  html += "    alert('Passwords do not match!');";
  html += "    return;";
  html += "  }";
  html += "  if (newPassword.length < 4) {";
  html += "    alert('Password must be at least 4 characters long!');";
  html += "    return;";
  html += "  }";
  html += "  if (confirm('Are you sure you want to change the web password? You will need to log in again with the new password.')) {";
  html += "    fetch('/save-password', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'password=' + encodeURIComponent(newPassword)}).then(response => response.text()).then(data => {";
  html += "      if (data.includes('SUCCESS')) {";
  html += "        alert('Password changed successfully! Please log in again with your new password.');";
  html += "        window.location.href = '/';";
  html += "      } else {";
  html += "        alert('Error: ' + data);";
  html += "      }";
  html += "    });";
  html += "  }";
  html += "}";
  html += "</script>";

  html += "</div>";  // Close container
  html += getFooter();
  html += "</body></html>";
  server.send(200, "text/html; charset=UTF-8", html);
}

#endif // WEB_PAGES_ADMIN_SECURITY_H
