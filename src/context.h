#pragma once
#include "structGlobal.h"
#include "esp_http_server.h"

//-----------------------------------------------------------------------
// I2S pin definitions based on ESP32 chip variant
//-----------------------------------------------------------------------
#ifdef CONFIG_IDF_TARGET_ESP32
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S3
#define I2S_DOUT 40
#define I2S_BCLK 42
#define I2S_LRC 41
#endif

#ifdef CONFIG_IDF_TARGET_ESP32P4
#define I2S_DOUT 22
#define I2S_BCLK 20
#define I2S_LRC 21
#endif

class Context
{
private:
    // Private constructor — implemented inline
    Context()
    {
        // TAGS------------------------------------------------------
        TAG_HTTP_SERVER = "HTTP_SERVER";
        TAG_WIFI = "WIFI_UTIL";
        TAG_AUDIO = "AUDIO_UTIL";
        TAG_SPEECH = "SPEECH_UTIL";
        TAG_FILE_SERVER = "FILE_SERVER";
        TAG_SERVER_UTIL = "SERVER_UTIL";
        TAG_FS_UTIL = "FS_UTIL";
        TAG_ESP32_AUDIO_SERVER = "ESP32_AUDIO_SERVER";

        // HTTP Constants---------------------------------------------
        MAX_QUERY_LEN = 512;
        MAX_LINK_LEN = 512;
        MAX_RESP_LEN = 512;
        http_stack_size = 8192 * 2;

        server = nullptr;

        http_uri_websocket = "/ws";
        http_uri_root = "/";
        http_uri_speech = "/speech";
        http_uri_lspeech = "/lspeech";
        http_uri_static_file_upload = "/upload";
        http_uri_static_file_serve = "/home";
        http_uri_static_file_delete = "/delete";
        http_uri_get_config = "/cfg";
        http_uri_set_config = "/scfg";

        mdns_service_name = "audio";

        // Speech Util Dynamics----------------------------------------
        largs = nullptr;
        args = nullptr;

        // Wifi config-------------------------------------------------

        ssid = "";                          // Default credentials for connecting to a AP
        password = "";

        ap_ssid = "Audio_Module";           // SoftAP name and password
        ap_password = "Audio_Module";

        wifiAPScanRetryTimeout = 15000;     // TIME BEFORE CHANGING TO SOFT AP IF NOT CONNECTED TO A KNOWN WIFI;
        wifiRetryScanTime = 30000;          // TIME BETWEEN SCANS FOR WIFI APs WHEN IN SOFT AP MODE

        // Speech Config Dynamics---------------------------------------
        local_tts_host = "192.168.154.220"; ///< Default TTS server IP
        local_tts_port = 5000;              ///< Default TTS server port
        local_tts_path = "/edge_tts";       ///< Default TTS request path

        google_speech_chunk_size = 150; ///< Default chunk size for Google TTS
        local_speech_chunk_size = 550;  ///< Default chunk size for local TTS
    }

    // Prevent copy and assignment
    Context(const Context &) = delete;
    Context &operator=(const Context &) = delete;

public:
    static Context *getInstance()
    {
        static Context instance; // Thread-safe since C++11
        return &instance;
    }

    //-----------------------------------------------------------------------
    // TAGS
    //-----------------------------------------------------------------------
    const char *TAG_HTTP_SERVER;
    const char *TAG_WIFI;
    const char *TAG_AUDIO;
    const char *TAG_SPEECH;
    const char *TAG_FILE_SERVER;
    const char *TAG_SERVER_UTIL;
    const char *TAG_FS_UTIL;
    const char *TAG_ESP32_AUDIO_SERVER;

    //-----------------------------------------------------------------------
    // HTTP Constants
    //-----------------------------------------------------------------------
    int MAX_QUERY_LEN;
    int MAX_LINK_LEN;
    int MAX_RESP_LEN;
    int http_stack_size;

    httpd_handle_t server;

    const char *http_uri_websocket;
    const char *http_uri_root;
    const char *http_uri_speech;
    const char *http_uri_lspeech;
    const char *http_uri_static_file_upload;
    const char *http_uri_static_file_serve;
    const char *http_uri_static_file_delete;
    const char *http_uri_get_config;
    const char *http_uri_set_config;

    const char *mdns_service_name;

    //-----------------------------------------------------------------------
    // Speech Util Dynamics
    //-----------------------------------------------------------------------
    lSynthesizeArgs *largs;
    SynthesizeArgs *args;

    //-----------------------------------------------------------------------
    // Wifi Config
    //-----------------------------------------------------------------------
    const char *ssid;
    const char *password;

    const char *ap_ssid;
    const char *ap_password;

    int wifiAPScanRetryTimeout;
    int wifiRetryScanTime;

    //-----------------------------------------------------------------------
    // Local Speech Config
    //-----------------------------------------------------------------------
    std::string local_tts_host;
    int local_tts_port;
    std::string local_tts_path;

    int google_speech_chunk_size; 
    int local_speech_chunk_size;
};
