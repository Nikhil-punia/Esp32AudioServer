#include "wifi_util.h"

 
WifiUtil* WifiUtil::getInstance() {
    static WifiUtil instance;
    return &instance;
}

WifiUtil::WifiUtil():ctx(Context::getInstance()){
    lastScan = 0;
    wifiRetryTimeout = ctx->wifiAPScanRetryTimeout;
    WiFi.onEvent(WiFiEvent);
    wifiState = WIFI_CONNECTING;
}

void  WifiUtil::storeWifiCred(const char *ssid, const char *password)
{
    std::pair<cJSON *, cJSON *> root = fsUtil.getObjectFromRoot(fsUtil.root_file_path, "wifi");
    cJSON *wifiArr = cJSON_GetObjectItem(root.second, "hosts");

    if (!wifiArr)
    {
        wifiArr = cJSON_CreateArray();
        cJSON_AddItemToObject(root.second, "hosts", wifiArr);
    }

    // Check if SSID already exists, update password if so
    bool found = false;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, wifiArr)
    {
        cJSON *ssidItem = cJSON_GetObjectItem(item, "ssid");
        if (ssidItem && strcmp(ssidItem->valuestring, ssid) == 0)
        {
            cJSON_ReplaceItemInObject(item, "password", cJSON_CreateString(password));
            found = true;
            break;
        }
    }
    if (!found)
    {
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddStringToObject(entry, "ssid", ssid);
        cJSON_AddStringToObject(entry, "password", password);
        cJSON_AddItemToArray(wifiArr, entry);
    }

    char *jsonStr = cJSON_Print(root.first);
    fsUtil.writeToFile(fsUtil.root_file_path, jsonStr);
    if (jsonStr)
        free(jsonStr);
    cJSON_Delete(root.first);
}

void WifiUtil::connectToBestWifiOrSoftAP(const char *ap_ssid, const char *ap_password)
{
    g_ap_ssid = ap_ssid;
    g_ap_password = ap_password;

    std::pair<cJSON *, cJSON *> root = fsUtil.getObjectFromRoot(fsUtil.root_file_path, "wifi");
    cJSON *wifiArr = cJSON_GetObjectItem(root.second, "hosts");

    int n = WiFi.scanNetworks();
    int bestRssi = -1000;
    bestSsid = "";
    bestPass = "";

    if (wifiArr && cJSON_IsArray(wifiArr))
    {
        for (int i = 0; i < n; ++i)
        {
            String foundSsid = WiFi.SSID(i);
            cJSON *item = NULL;
            cJSON_ArrayForEach(item, wifiArr)
            {
                cJSON *ssidItem = cJSON_GetObjectItem(item, "ssid");
                cJSON *passItem = cJSON_GetObjectItem(item, "password");
                if (ssidItem && passItem && foundSsid == ssidItem->valuestring)
                {
                    int rssi = WiFi.RSSI(i);
                    if (rssi > bestRssi)
                    {
                        bestRssi = rssi;
                        bestSsid = foundSsid;
                        bestPass = passItem->valuestring;
                    }
                }
            }
        }
    }
    if (root.first)
        cJSON_Delete(root.first);

    if (bestSsid.length() > 0)
    {
        if (wifiStateConfirm == "WIFI_SOFTAP")
        {
            Serial.println("Stopping SoftAP before connecting to WiFi STA...");
            WiFi.softAPdisconnect(true);
            delay(100); // Give some time for the SoftAP to stop
        }

        Serial.printf("Connecting to best known WiFi: %s (RSSI: %d)\n", bestSsid.c_str(), bestRssi);
        WiFi.mode(WIFI_STA);
        WiFi.begin(bestSsid.c_str(), bestPass.c_str());
        wifiConnectStart = millis();
        wifiState = WIFI_CONNECTING;
    }
    else
    {
        wifiConnectStart = millis();
        wifiState = WIFI_CONNECTING; // Will fallback to SoftAP after timeout
    }
}

void WifiUtil::wifiManagerLoop()
{
    if (wifiState == WIFI_CONNECTING)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("\nWiFi connected");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
            wifiState = WIFI_CONNECTED;
            wifiStateConfirm = "WIFI_STA";
        }
        else if ((millis() - wifiConnectStart > wifiRetryTimeout) && wifiStateConfirm != "WIFI_SOFTAP")
        {
            Serial.println("\nWiFi connect timeout, starting SoftAP...");
            WiFi.mode(WIFI_MODE_AP);
            WiFi.softAP(g_ap_ssid ? g_ap_ssid : "Audio_Module", g_ap_password ? g_ap_password : "Audio_Module");
            Serial.print("SoftAP started. IP address: ");
            Serial.println(WiFi.softAPIP());
            lastScan = millis();
            wifiState = WIFI_SOFTAP;
            wifiStateConfirm = "WIFI_SOFTAP";
        }
        // Do NOT call connectToBestWifiOrSoftAP() repeatedly here!
    }

    if (wifiStateConfirm == "WIFI_SOFTAP")
    {
        // Optionally, periodically scan and try to reconnect to WiFi

        if (millis() - lastScan > ctx->wifiRetryScanTime)
        { 
            lastScan = millis();
            Serial.println("\nChecking If Wifi Is Available");
            connectToBestWifiOrSoftAP("Audio_Module", "Audio_Module");
        }
    }
}

void WifiUtil::WiFiEvent(WiFiEvent_t event) {
    WifiUtil* self = WifiUtil::getInstance();

    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        if (self->disconnectReset) {
            Serial.println("WiFi disconnected! Attempting to reconnect...");
            self->wifiState = WIFI_CONNECTING;
            self->wifiConnectStart = millis();
            self->disconnectReset = false;
        }
    }
    else if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
        self->disconnectReset = true;
    }
}