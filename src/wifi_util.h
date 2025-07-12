#pragma once

#include "WiFi.h"
#include "esp_log.h"
#include "cJSON.h"
#include "fs_util.h"
#include "context.h"

/**
 * @brief WiFi utility class for managing Wi-Fi connection and SoftAP fallback.
 *
 * This class provides Wi-Fi connection logic for the ESP32, supporting:
 * - Connecting to the best-known Wi-Fi network from stored credentials.
 * - Falling back to SoftAP mode for setup when no known networks are available.
 * - Handling Wi-Fi events via a static callback.
 *
 * @note Wi-Fi credentials are currently stored in the filesystem (FFAT).
 *       This allows easy JSON-based updates and debugging.
 *       ⚠️ TODO: For production, consider migrating to NVS for secure,
 *       reliable, and transactional storage of credentials.
 */
class WifiUtil
{

private:
    Context *ctx;      ///< Shared configuration context

    enum WifiState
    {
        WIFI_IDLE,
        WIFI_CONNECTING,
        WIFI_CONNECTED,
        WIFI_SOFTAP
    };

    WifiUtil();                                 ///< Private constructor
    WifiUtil(const WifiUtil&) = delete;
    WifiUtil& operator=(const WifiUtil&) = delete;

    // ------------------ Internal State Tracking --------------------

    WifiState wifiState = WIFI_IDLE;
    String wifiStateConfirm = "WIFI_IDLE";

    unsigned long wifiConnectStart = 0;
    unsigned long lastScan = 0;
    unsigned long wifiRetryTimeout = 0;

    String bestSsid = "";                       ///< Best-matching SSID found during scan
    String bestPass = "";                       ///< Password for the best-matching SSID

    const char *g_ap_ssid = nullptr;
    const char *g_ap_password = nullptr;

    bool disconnectReset = false;               ///< Track disconnection-triggered resets

public:
    /**
     * @brief Stores Wi-Fi credentials (SSID & password) to FFAT as JSON.
     * 
     * This method saves the Wi-Fi credentials in a config file (usually at /spiflash/config.json),
     * managed by FsUtil. You can retrieve and edit them later for debugging or recovery.
     *
     * @param ssid Wi-Fi SSID to store
     * @param password Corresponding Wi-Fi password
     */
    void storeWifiCred(const char *ssid, const char *password);

    /**
     * @brief Attempts to connect to the strongest saved Wi-Fi or starts SoftAP.
     *
     * This function scans for available Wi-Fi networks and compares SSIDs to
     * the stored credentials. If no match is found or connection fails,
     * it starts a SoftAP for configuration.
     *
     * @param ap_ssid SSID of the SoftAP mode to fall back to
     * @param ap_password Password of the SoftAP
     */
    void connectToBestWifiOrSoftAP(const char *ap_ssid, const char *ap_password);

    /**
     * @brief Should be called periodically to manage Wi-Fi state transitions.
     */
    void wifiManagerLoop();

    /**
     * @brief Static event handler for Wi-Fi events.
     * 
     * Used internally to track and respond to Wi-Fi state changes.
     */
    static void WiFiEvent(WiFiEvent_t event);

    /**
     * @brief Returns the singleton instance of WifiUtil.
     */
    static WifiUtil* getInstance();

    // ------------------ Configuration (from context) --------------------

    // These fields are dynamically loaded from the shared Context instance.
    // They represent currently active Wi-Fi credentials loaded from FFAT (via FsUtil).
    const char *ssid = ctx->ssid;               ///< Current SSID
    const char *password = ctx->password;       ///< Current Wi-Fi password

    const char *ap_ssid = ctx->ap_ssid;         ///< SoftAP fallback SSID
    const char *ap_password = ctx->ap_password; ///< SoftAP fallback password

};

