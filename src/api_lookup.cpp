/*
 * api_lookup.cpp - DMR ID and User Information Lookup
 *
 * Implementation of API lookup functions for DMR callsigns and user information
 */

#include "../include/api_lookup.h"

String lookupUserInfo(uint32_t dmrId) {
  if (dmrId == 0) return "";

  // Check for special DMR system IDs first (these won't be in RadioID database)
  switch (dmrId) {
    case 9990: return "Echo|Echo Test|Parrot|Service";
    case 4000: return "Disconnect|Disconnect|Service|Command";
    case 5000: return "Status|Status Check|Service|Command";
    case 8045: return "Time|Time Server|Service|Server";
    case 9000: return "NXDN|NXDN Reflector|Service|Server";
    case 9099: return "Info|Information|Service|Server";
    default: break;  // Continue with normal lookup
  }

  // Check cache first
  String cached = getCachedUserInfo(dmrId);
  if (cached.length() > 0) {
    return cached;
  }

  // Not in cache, try API lookup
  String userInfo = lookupUserInfoAPI(dmrId);

  // Cache the result (even if empty to avoid repeated failed lookups)
  if (userInfo.length() > 0) {
    cacheUserInfo(dmrId, userInfo);
  }

  return userInfo;
}

// Legacy callsign lookup function - checks cache first, then API
String lookupCallsign(uint32_t dmrId) {
  if (dmrId == 0) return "";

  // Try enhanced lookup first
  String userInfo = lookupUserInfo(dmrId);
  if (userInfo.length() > 0) {
    int pipeIndex = userInfo.indexOf('|');
    return (pipeIndex > 0) ? userInfo.substring(0, pipeIndex) : userInfo;
  }

  // Fallback to legacy cache
  String cached = getCachedCallsign(dmrId);
  if (cached.length() > 0) {
    return cached;
  }

  // Not in cache, try API lookup
  String callsign = lookupCallsignAPI(dmrId);

  // Cache the result (even if empty to avoid repeated failed lookups)
  if (callsign.length() > 0) {
    cacheCallsign(dmrId, callsign);
  }

  return callsign;
}

// Check if user info is in cache
String getCachedUserInfo(uint32_t dmrId) {
  for (int i = 0; i < DMR_USER_CACHE_SIZE; i++) {
    if (userCache[i].dmrId == dmrId && userCache[i].callsign.length() > 0) {
      // Return formatted user info string: "callsign|name|city|country"
      return userCache[i].callsign + "|" + userCache[i].name + "|" + userCache[i].city + "|" + userCache[i].country;
    }
  }
  return "";
}

// Add user info to cache (circular buffer)
void cacheUserInfo(uint32_t dmrId, String userInfo) {
  // Parse userInfo string: "callsign|name|city|country"
  int firstPipe = userInfo.indexOf('|');
  int secondPipe = userInfo.indexOf('|', firstPipe + 1);
  int thirdPipe = userInfo.indexOf('|', secondPipe + 1);

  userCache[userCacheIndex].dmrId = dmrId;
  userCache[userCacheIndex].callsign = (firstPipe > 0) ? userInfo.substring(0, firstPipe) : "";
  userCache[userCacheIndex].name = (secondPipe > firstPipe) ? userInfo.substring(firstPipe + 1, secondPipe) : "";
  userCache[userCacheIndex].city = (thirdPipe > secondPipe) ? userInfo.substring(secondPipe + 1, thirdPipe) : "";
  userCache[userCacheIndex].country = (thirdPipe > 0) ? userInfo.substring(thirdPipe + 1) : "";
  userCache[userCacheIndex].timestamp = millis();
  userCacheIndex = (userCacheIndex + 1) % DMR_USER_CACHE_SIZE;
}

// Check if callsign is in legacy cache
String getCachedCallsign(uint32_t dmrId) {
  for (int i = 0; i < DMR_CALLSIGN_CACHE_SIZE; i++) {
    if (callsignCache[i].dmrId == dmrId && callsignCache[i].callsign.length() > 0) {
      return callsignCache[i].callsign;
    }
  }
  return "";
}

// Add callsign to legacy cache (circular buffer)
void cacheCallsign(uint32_t dmrId, String callsign) {
  callsignCache[callsignCacheIndex].dmrId = dmrId;
  callsignCache[callsignCacheIndex].callsign = callsign;
  callsignCache[callsignCacheIndex].timestamp = millis();
  callsignCacheIndex = (callsignCacheIndex + 1) % DMR_CALLSIGN_CACHE_SIZE;
}

// Enhanced user info lookup via RadioID.net API
String lookupUserInfoAPI(uint32_t dmrId) {
  if (!wifiConnected) {
    return "";
  }

  HTTPClient http;
  String url = String(DMR_API_URL) + String(dmrId);

  http.begin(url);
  http.setTimeout(DMR_API_TIMEOUT);  // API timeout from config.h

  int httpCode = http.GET();
  String userInfo = "";

  if (httpCode == 200) {
    String payload = http.getString();

    // RadioID.net returns JSON: {"count":1,"results":[{"id":2041152,"callsign":"PA3ANG","fname":"John","name":"John","city":"Amsterdam","country":"Netherlands",...}]}
    // Parse multiple fields: callsign, name/fname, city, country
    String callsign = "";
    String name = "";
    String city = "";
    String country = "";

    // Extract callsign
    int csIndex = payload.indexOf("\"callsign\":\"");
    if (csIndex > 0) {
      csIndex += 12;  // Length of "callsign":"
      int endIndex = payload.indexOf("\"", csIndex);
      if (endIndex > csIndex) {
        callsign = payload.substring(csIndex, endIndex);
      }
    }

    // Extract name (prefer 'name' over 'fname')
    int nameIndex = payload.indexOf("\"name\":\"");
    if (nameIndex > 0) {
      nameIndex += 8;  // Length of "name":"
      int endIndex = payload.indexOf("\"", nameIndex);
      if (endIndex > nameIndex) {
        name = payload.substring(nameIndex, endIndex);
        if (name == "null" || name.length() == 0) {
          // Try fname if name is null/empty
          int fnameIndex = payload.indexOf("\"fname\":\"");
          if (fnameIndex > 0) {
            fnameIndex += 9;  // Length of "fname":"
            int fendIndex = payload.indexOf("\"", fnameIndex);
            if (fendIndex > fnameIndex) {
              name = payload.substring(fnameIndex, fendIndex);
            }
          }
        }
      }
    }

    // Extract city
    int cityIndex = payload.indexOf("\"city\":\"");
    if (cityIndex > 0) {
      cityIndex += 8;  // Length of "city":"
      int endIndex = payload.indexOf("\"", cityIndex);
      if (endIndex > cityIndex) {
        city = payload.substring(cityIndex, endIndex);
        if (city == "null") city = "";
      }
    }

    // Extract country
    int countryIndex = payload.indexOf("\"country\":\"");
    if (countryIndex > 0) {
      countryIndex += 11;  // Length of "country":"
      int endIndex = payload.indexOf("\"", countryIndex);
      if (endIndex > countryIndex) {
        country = payload.substring(countryIndex, endIndex);
        if (country == "null") country = "";
      }
    }

    // Build userInfo string: "callsign|name|city|country"
    if (callsign.length() > 0) {
      userInfo = callsign;
      if (name.length() > 0 || city.length() > 0 || country.length() > 0) {
        userInfo += "|" + name + "|" + city + "|" + country;
      }
    }
  } else if (httpCode > 0) {
    logSerial("[API] User info lookup failed: HTTP " + String(httpCode));
  }

  http.end();
  return userInfo;
}

// Legacy callsign lookup via RadioID.net API
String lookupCallsignAPI(uint32_t dmrId) {
  // Use enhanced lookup and extract just the callsign
  String userInfo = lookupUserInfoAPI(dmrId);
  if (userInfo.length() > 0) {
    int pipeIndex = userInfo.indexOf('|');
    return (pipeIndex > 0) ? userInfo.substring(0, pipeIndex) : userInfo;
  }
  return "";
}
