#pragma once

#include <WiFi.h>

inline bool hasActiveStationWifiConnection() {
  return (WiFi.getMode() & WIFI_MODE_STA) && WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0);
}
