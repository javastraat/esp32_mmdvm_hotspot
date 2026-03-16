// filesystem.ino — LittleFS initialisation.
// Global fsAvailable declared in ulanzi-espnow.ino.

static void setupFilesystem() {
  if (!LittleFS.begin(true)) {    // true = format on first boot if empty
    Serial.println("[FS] LittleFS mount failed — filesystem unavailable");
    return;
  }
  fsAvailable = true;
  Serial.printf("[FS] LittleFS mounted — %u KB total, %u KB used\n",
                LittleFS.totalBytes() / 1024,
                LittleFS.usedBytes()  / 1024);
}
