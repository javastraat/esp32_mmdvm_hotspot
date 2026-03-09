/*
 * bm_servers.h - BrandMeister server list for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_COMMON_SERVER_UTILS_H
#define WEB_COMMON_SERVER_UTILS_H

#include <Arduino.h>

struct BMServer {
  const char* address;
  const char* name;
};

const BMServer bmServers[] = {
  { "2041.master.brandmeister.network", "BM_2041_Netherlands" },
  { "api.brandmeister.network", "BM_Main_BrandMeister_Hub" },
  { "44.148.230.201", "BM_2001_Europe_HAMNET" },
  { "2022.master.brandmeister.network", "BM_2022_Greece" },
  { "2061.master.brandmeister.network", "BM_2061_Belgium" },
  { "2081.master.brandmeister.network", "BM_2081_France" },
  { "2082.master.brandmeister.network", "BM_2082_France" },
  { "2141.master.brandmeister.network", "BM_2141_Spain" },
  { "2162.master.brandmeister.network", "BM_2162_Hungary" },
  { "2222.master.brandmeister.network", "BM_2222_Italy" },
  { "2262.master.brandmeister.network", "BM_2262_Romania" },
  { "2282.master.brandmeister.network", "BM_2282_Switzerland" },
  { "2302.master.brandmeister.network", "BM_2302_Czech_Republic" },
  { "2322.master.brandmeister.network", "BM_2322_Austria" },
  { "2341.master.brandmeister.network", "BM_2341_United_Kingdom" },
  { "2382.master.brandmeister.network", "BM_2382_Denmark" },
  { "2402.master.brandmeister.network", "BM_2402_Sweden" },
  { "2421.master.brandmeister.network", "BM_2421_Norway" },
  { "2441.master.brandmeister.network", "BM_2441_Finland" },
  { "2502.master.brandmeister.network", "BM_2502_Russia" },
  { "2503.master.brandmeister.network", "BM_2503_Russia" },
  { "23.111.17.39", "BM_2551_Ukraine" },
  { "2602.master.brandmeister.network", "BM_2602_Poland" },
  { "2621.master.brandmeister.network", "BM_2621_Germany" },
  { "2622.master.brandmeister.network", "BM_2622_Germany" },
  { "2682.master.brandmeister.network", "BM_2682_Portugal" },
  { "2721.master.brandmeister.network", "BM_2721_Ireland" },
  { "2841.master.brandmeister.network", "BM_2841_Bulgaria" },
  { "2931.master.brandmeister.network", "BM_2931_Slovenia" },
  { "3021.master.brandmeister.network", "BM_3021_Canada" },
  { "3102.master.brandmeister.network", "BM_3102_United_States" },
  { "3103.master.brandmeister.network", "BM_3103_United_States" },
  { "3104.master.brandmeister.network", "BM_3104_United_States" },
  { "3341.master.brandmeister.network", "BM_3341_Mexico" },
  { "4251.master.brandmeister.network", "BM_4251_Israel" },
  { "4501.master.brandmeister.network", "BM_4501_South_Korea" },
  { "4602.master.brandmeister.network", "BM_4602_China" },
  { "5021.master.brandmeister.network", "BM_5021_Malaysia" },
  { "5051.master.brandmeister.network", "BM_5051_Australia" },
  { "5151.master.brandmeister.network", "BM_5151_Philippines" },
  { "6551.master.brandmeister.network", "BM_6551_South_Africa" },
  { "7242.master.brandmeister.network", "BM_7242_Brazil" },
  { "7301.master.brandmeister.network", "BM_7301_Chile" }
};
const int bmServerCount = sizeof(bmServers) / sizeof(bmServers[0]);

// Helper function to get friendly server name from address
String getServerDisplayName(String serverAddress) {
  for (int i = 0; i < bmServerCount; i++) {
    if (serverAddress == bmServers[i].address) {
      return String(bmServers[i].name);
    }
  }
  return serverAddress;
}

#endif  // WEB_COMMON_SERVER_UTILS_H
