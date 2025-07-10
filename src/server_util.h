#pragma once

#include "esp_http_server.h"
#include "esp_log.h"
#include "audio_util.h"
#include <ESPmDNS.h>
#include "cJSON.h"
#include <string>
#include <vector>
#include "speech_util.h"
#include "file_server.h"
#include <esp_psram.h>
#include "esp_wifi.h"
#include <esp_clk_tree.h>
#include "esp_flash.h"
#include <esp_chip_info.h>
#include "esp_private/esp_clk.h"
#include "driver/temperature_sensor.h"

class ServerUtil
{
private:
    httpd_uri_t root_uri;
    httpd_uri_t speech_uri;
    httpd_uri_t lspeech_uri;
    httpd_uri_t static_file_upload;
    httpd_uri_t static_file_serve;
    httpd_uri_t static_file_delete;
    httpd_uri_t get_config;
    httpd_uri_t set_config;
    httpd_uri_t websocket_uri;

    static esp_err_t root_get_handler(httpd_req_t *req);     // ✅ static
    static esp_err_t speech_post_handler(httpd_req_t *req);  // ✅ static
    static esp_err_t lspeech_post_handler(httpd_req_t *req); // ✅ static
    static esp_err_t config_get_handler(httpd_req_t *req);   // ✅ static
    static esp_err_t set_config_handler(httpd_req_t *req);  // ✅ static
    static void handleValueForConfig(httpd_req_t *req, ServerUtil *self, const char *cfg, const char *query, const char *value);
    static void sendJsonResponse(httpd_req_t *req, cJSON *json_response, ServerUtil *self);
    static void handleQueryForConfig(httpd_req_t *req, ServerUtil *self, const char *query,bool webs);
    static void sendJsonWebResponse(httpd_req_t *req, cJSON *json_response, ServerUtil *self);
    static float getTemperature();
    static esp_err_t websocket_handler(httpd_req_t *req) ;
    static void sendStringWebResponse(httpd_req_t *req, char *json_string, ServerUtil *self);
    static void handleQueryForWebSettings(httpd_req_t *req, ServerUtil *self);

public:
    ServerUtil(SpeechUtil *speechUtilPtr, AudioUtil *audioUtilPtr);

    SpeechUtil *speechUtil;
    AudioUtil *audioUtil;
   

    struct SynthesizeArgs
    {
        SpeechUtil *speechUtil;
        AudioUtil *audioUtil;
        std::string *text;
        std::string *lang;
    };

    struct lSynthesizeArgs
    {
        SpeechUtil *speechUtil;
        AudioUtil *audioUtil;
        std::unique_ptr<std::string> text;
        std::unique_ptr<std::string> voice_id;
    };

    lSynthesizeArgs *largs = nullptr;
    SynthesizeArgs *args = nullptr;

    int MAX_QUERY_LEN = 512;
    int MAX_LINK_LEN = 512;
    int MAX_RESP_LEN = 512;
    const char *TAG = "HTTP_SERVER";

    static bool stopAllPreviousTasks(ServerUtil *self);

    httpd_handle_t start_webserver(void);
};

extern ServerUtil serverUtil;