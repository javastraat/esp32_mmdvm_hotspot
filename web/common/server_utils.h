/*
 * server_utils.h - Server utility functions for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_COMMON_SERVER_UTILS_H
#define WEB_COMMON_SERVER_UTILS_H

#include <Arduino.h>

// Helper function to get friendly server name
String getServerDisplayName(String serverAddress) {
  // BrandMeister server list
  struct BMServer {
    const char* address;
    const char* name;
  };

  const BMServer bmServers[] = {
    {"2041.master.brandmeister.network", "BM_2041_Netherlands"},
    {"44.148.230.201", "BM_2001_Europe_HAMNET"},
    {"2022.master.brandmeister.network", "BM_2022_Greece"},
    {"2061.master.brandmeister.network", "BM_2061_Belgium"},
    {"2081.master.brandmeister.network", "BM_2081_France"},
    {"44.131.4.1", "BM_3100_USA"},
    {"3102.master.brandmeister.network", "BM_3102_Canada"},
    {"3106.master.brandmeister.network", "BM_3106_Mexico"},
    {"2021.master.brandmeister.network", "BM_2021_Spain"},
    {"2028.master.brandmeister.network", "BM_2028_Portugal"},
    {"2029.master.brandmeister.network", "BM_2029_UK"},
    {"2032.master.brandmeister.network", "BM_2032_Ireland"},
    {"2042.master.brandmeister.network", "BM_2042_Germany"},
    {"2043.master.brandmeister.network", "BM_2043_Austria"},
    {"2045.master.brandmeister.network", "BM_2045_Denmark"},
    {"2046.master.brandmeister.network", "BM_2046_Sweden"},
    {"2047.master.brandmeister.network", "BM_2047_Norway"},
    {"2050.master.brandmeister.network", "BM_2050_Russia"},
    {"2051.master.brandmeister.network", "BM_2051_Poland"},
    {"2062.master.brandmeister.network", "BM_2062_Italy"},
    {"2063.master.brandmeister.network", "BM_2063_Switzerland"},
    {"5053.master.brandmeister.network", "BM_5053_Australia"},
    {"5055.master.brandmeister.network", "BM_5055_New_Zealand"},
    {"4501.master.brandmeister.network", "BM_4501_Japan"},
    {"4502.master.brandmeister.network", "BM_4502_South_Korea"},
    {"4601.master.brandmeister.network", "BM_4601_Taiwan"},
    {"5151.master.brandmeister.network", "BM_5151_Philippines"},
    {"6551.master.brandmeister.network", "BM_6551_South_Africa"},
    {"7242.master.brandmeister.network", "BM_7242_Brazil"},
    {"7301.master.brandmeister.network", "BM_7301_Chile"}
  };
  const int serverCount = sizeof(bmServers) / sizeof(bmServers[0]);

  // Look up friendly name
  for (int i = 0; i < serverCount; i++) {
    if (serverAddress == bmServers[i].address) {
      return String(bmServers[i].name);
    }
  }

  // If not found, return the original address
  return serverAddress;
}

#endif // WEB_COMMON_SERVER_UTILS_H
