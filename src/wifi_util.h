#pragma once

#include "WiFi.h"
#include "esp_log.h"
#include "cJSON.h"
#include "fs_util.h"

class WifiUtil
{

public:
    void storeWifiCred(const char *ssid, const char *password);
    void connectToBestWifiOrSoftAP(const char *ap_ssid, const char *ap_password);
    void wifiManagerLoop();
    static void WiFiEvent(WiFiEvent_t event);

    static WifiUtil* getInstance(); // Lazy singleton accessor

    // WiFi Credentials - IMPORTANT: For production, consider storing these securely (e.g., in NVS)
    const char *ssid = "";     // Your WiFi SSID
    const char *password = ""; // Your WiFi password

    // SoftAP credentials
    const char *ap_ssid = "Audio_Module";
    const char *ap_password = "Audio_Module";

private:
    enum WifiState
    {
        WIFI_IDLE,
        WIFI_CONNECTING,
        WIFI_CONNECTED,
        WIFI_SOFTAP
    };

    WifiUtil(); // Already defined
    WifiUtil(const WifiUtil&) = delete;
    WifiUtil& operator=(const WifiUtil&) = delete;

    WifiState wifiState = WIFI_IDLE;
    String wifiStateConfirm = "WIFI_IDLE";
    unsigned long wifiConnectStart = 0;
    unsigned long lastScan;
    unsigned long wifiRetryTimeout;
    String bestSsid = "";
    String bestPass = "";
    const char *g_ap_ssid = nullptr;
    const char *g_ap_password = nullptr;
    bool disconnectReset = false;
};