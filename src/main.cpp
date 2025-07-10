#include "Arduino.h"
#include "Audio.h"
#include "WiFi.h"
#include "esp_http_server.h"
#include "audio_util.h"
#include "server_util.h"
#include "fs_util.h"
#include "file_server.h"
#include <cJSON.h>
#include "freertos/semphr.h"

FsUtil fsUtil;
SpeechUtil speechUtil;
AudioUtil audioUtil;
FileServer fileServer; // Instance of FileServer to handle static files

ServerUtil serverUtil(&speechUtil, &audioUtil);

// WiFi Credentials - IMPORTANT: For production, consider storing these securely (e.g., in NVS)
const char *ssid = "";            // Your WiFi SSID
const char *password = ""; // Your WiFi password

// SoftAP credentials
const char *ap_ssid = "Audio_Module";
const char *ap_password = "Audio_Module";

void storeWifiCred(const char *ssid, const char *password)
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

enum WifiState
{
    WIFI_IDLE,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_SOFTAP
};
WifiState wifiState = WIFI_IDLE;
String wifiStateConfirm = "WIFI_IDLE";
unsigned long wifiConnectStart = 0;
static unsigned long lastScan = 0;
const unsigned long wifiRetryTimeout = 15000; // 15 seconds to try connecting
String bestSsid = "";
String bestPass = "";
const char *g_ap_ssid = nullptr;
const char *g_ap_password = nullptr;
bool disconnectReset = false;

// --- Modified function: only scans and starts connection, does not block ---
void connectToBestWifiOrSoftAP(const char *ap_ssid, const char *ap_password)
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

void wifiManagerLoop()
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
        
        if (millis() - lastScan > 30000) { // Try every 30 seconds
            lastScan = millis();
            Serial.println("\nChecking If Wifi Is Available");
            connectToBestWifiOrSoftAP("Audio_Module", "Audio_Module");
        }
    }
}

void WiFiEvent(WiFiEvent_t event)
{
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
    {
        if (disconnectReset)
        {
            Serial.println("WiFi disconnected! Attempting to reconnect...");
            wifiState = WIFI_CONNECTING;
            wifiConnectStart = millis();
            disconnectReset = false;
        }
    }
    else if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED)
    {
        disconnectReset = true;
    }
}

/**
 * @brief Arduino setup function. Runs once at device startup.
 */
void setup()
{
    Serial.begin(115200); // Initialize serial communication for debugging output
    Serial.print("A\n\n");
    Serial.println("----------------------------------");
    Serial.printf("ESP32 Chip: %s\n", ESP.getChipModel());
    Serial.printf("Arduino Version: %d.%d.%d\n", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
    Serial.printf("ESP-IDF Version: %d.%d.%d\n", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
    Serial.printf("ARDUINO_LOOP_STACK_SIZE %d words (32 bit)\n", CONFIG_ARDUINO_LOOP_STACK_SIZE);
    Serial.println("----------------------------------");
    Serial.print("\n\n");

    fsUtil.checkAndInitialObjectInFile(fsUtil.root_file_path); // Ensure the config file exists and has the necessary structure

    storeWifiCred("", "");

    WiFi.onEvent(WiFiEvent);
    wifiState = WIFI_CONNECTING;
    connectToBestWifiOrSoftAP("Audio_Module", "Audio_Module");

    serverUtil.start_webserver();
    audioUtil.setupAudio();       // Initialize audio settings
    speechUtil.setupSpeechUtil(); // Initialize speech settings

    // Initial music playback when the device starts up
    // audioUtil.handle_music_request("https://stream.radioparadise.com/mp3-320");
}

/**
 * @brief Arduino loop function. Runs continuously in a dedicated FreeRTOS task.
 */
void loop()
{
    wifiManagerLoop();
    audioUtil.loopAudio(); // Handle audio playback logic in the main loop
    delay(1);
}

// Optional callback for general audio information messages
void audio_info(const char *info)
{
    Serial.print("info        ");
    Serial.println(info);
}

// Optional callback for audio warning messages (highly recommended for debugging)
void audio_warn(const char *warn)
{
    Serial.print("warn        ");
    Serial.println(warn);
}

// Optional callback for audio error messages (highly recommended for debugging)
void audio_error(const char *error)
{
    Serial.print("error       ");
    Serial.println(error);
}
