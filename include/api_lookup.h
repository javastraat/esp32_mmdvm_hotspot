/*
 * api_lookup.h - DMR ID API Lookup and Caching for ESP32 MMDVM Hotspot
 *
 * Handles DMR callsign and user info lookup via RadioID API with local caching
 *
 * NOTE: Function implementations remain in main .ino file
 * This header provides declarations and extern variables for API lookup functions
 */

#ifndef API_LOOKUP_H
#define API_LOOKUP_H

#include <Arduino.h>
#include <HTTPClient.h>
#include "config.h"

// Cache structures
struct UserCache {
  uint32_t dmrId;
  String callsign;
  String name;
  String city;
  String country;
  unsigned long timestamp;
};

struct CallsignCache {
  uint32_t dmrId;
  String callsign;
  unsigned long timestamp;
};

// External cache arrays (defined in main .ino)
extern UserCache userCache[DMR_USER_CACHE_SIZE];
extern int userCacheIndex;
extern CallsignCache callsignCache[DMR_CALLSIGN_CACHE_SIZE];
extern int callsignCacheIndex;

// External logging function
extern void logSerial(String message);
extern void logSerialVerbose(String message);

// External debug flags
extern bool debug_network;

// Function declarations
String lookupCallsign(uint32_t dmrId);
String lookupCallsignAPI(uint32_t dmrId);
String lookupUserInfo(uint32_t dmrId);
String lookupUserInfoAPI(uint32_t dmrId);
String getCachedCallsign(uint32_t dmrId);
String getCachedUserInfo(uint32_t dmrId);

#endif // API_LOOKUP_H
