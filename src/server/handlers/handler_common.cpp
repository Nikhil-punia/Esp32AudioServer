#include "handler_common.h"
#include "context.h"
#include "audio_util.h"
#include "cpu/temperature_util.h"

#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_flash.h"
#include "esp_psram.h"
#include "esp_private/esp_clk.h"
 
#include "esp_heap_caps.h"
#include <cJSON.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// ------------- JSON RESPONSES -------------

void sendJsonResponse(httpd_req_t *req, cJSON *json_response)
{
    Context *ctx = Context::getInstance();

    if (!json_response) {
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "JSON response is NULL");
        httpd_resp_send_500(req);
        return;
    }

    char *json_str = cJSON_Print(json_response);
    if (!json_str) {
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "Failed to serialize JSON response");
        cJSON_Delete(json_response);
        httpd_resp_send_500(req);
        return;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    esp_err_t res = httpd_resp_sendstr(req, json_str);
    free(json_str);
    cJSON_Delete(json_response);

    if (res != ESP_OK)
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "Failed to send response");
}

void sendJsonWebResponse(httpd_req_t *req, cJSON *json_response)
{
    Context *ctx = Context::getInstance();

    if (!json_response) {
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "JSON response is NULL");
        return;
    }

    char *json_str = cJSON_PrintUnformatted(json_response);
    if (!json_str) {
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "Failed to serialize JSON response");
        cJSON_Delete(json_response);
        return;
    }

    httpd_ws_frame_t ws_res = {
        .final = true,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)json_str,
        .len = strlen(json_str),
    };

    esp_err_t res = httpd_ws_send_frame(req, &ws_res);
    free(json_str);
    cJSON_Delete(json_response);

    if (res != ESP_OK)
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "Failed to send websocket response");
}

void sendStringWebResponse(httpd_req_t *req, char *json_string)
{
    Context *ctx = Context::getInstance();

    if (!json_string) {
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "JSON response is NULL");
        return;
    }

    httpd_ws_frame_t ws_res = {
        .final = true,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)json_string,
        .len = strlen(json_string),
    };

    esp_err_t res = httpd_ws_send_frame(req, &ws_res);
    free(json_string);

    if (res != ESP_OK)
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "Failed to send websocket response");
}

// ------------- MODULAR DATA HANDLERS -------------

static void addSystemInfo(cJSON *json_response)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    const char *chip_model_str = "Unknown";
    switch (chip_info.model) {
        case CHIP_ESP32: chip_model_str = "ESP32"; break;
        case CHIP_ESP32S2: chip_model_str = "ESP32-S2"; break;
        case CHIP_ESP32S3: chip_model_str = "ESP32-S3"; break;
        case CHIP_ESP32C3: chip_model_str = "ESP32-C3"; break;
        case CHIP_ESP32H2: chip_model_str = "ESP32-H2"; break;
        case CHIP_ESP32C2: chip_model_str = "ESP32-C2"; break;
        case CHIP_ESP32C6: chip_model_str = "ESP32-C6"; break;
        case CHIP_ESP32C61: chip_model_str = "ESP32-C61"; break;
        case CHIP_ESP32P4: chip_model_str = "ESP32-C61"; break;
        case CHIP_ESP32C5: chip_model_str = "ESP32-C61"; break;
        case CHIP_POSIX_LINUX: chip_model_str = "ESP32-C61"; break;
    }

    cJSON_AddNumberToObject(json_response, "uptime", esp_timer_get_time() / 1000000);
    cJSON_AddStringToObject(json_response, "version", "1.0.0");
    cJSON_AddStringToObject(json_response, "build_time", __DATE__ " " __TIME__);
    cJSON_AddStringToObject(json_response, "sdk_version", esp_get_idf_version());
    cJSON_AddStringToObject(json_response, "chip_model", chip_model_str);
    cJSON_AddNumberToObject(json_response, "chip_revision", chip_info.revision);
    cJSON_AddNumberToObject(json_response, "cores", chip_info.cores);
    cJSON_AddNumberToObject(json_response, "cpu_freq_mhz", esp_clk_cpu_freq() / 1000000);
    cJSON_AddNumberToObject(json_response, "temperature", getTemperature());
    cJSON_AddNumberToObject(json_response, "features", chip_info.features);
    cJSON_AddNumberToObject(json_response, "free_heap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(json_response, "total_heap", heap_caps_get_total_size(MALLOC_CAP_DEFAULT));
    cJSON_AddNumberToObject(json_response, "min_free_heap", esp_get_minimum_free_heap_size());
    cJSON_AddNumberToObject(json_response, "psram_size", esp_psram_get_size());
    cJSON_AddNumberToObject(json_response, "free_psram", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    esp_flash_t *flash = esp_flash_default_chip;
    if (flash) {
        uint32_t flash_size = 0;
        if (esp_flash_get_size(flash, &flash_size) == ESP_OK)
            cJSON_AddNumberToObject(json_response, "flash_size", flash_size);
        else
            cJSON_AddStringToObject(json_response, "flash_size", "Unknown");
    } else {
        cJSON_AddStringToObject(json_response, "flash_size", "Unknown");
    }

    cJSON_AddStringToObject(json_response, "board", CONFIG_IDF_TARGET);
    cJSON_AddStringToObject(json_response, "chip", CONFIG_IDF_TARGET);
}

static void addWifiStatusInfo(cJSON *json_response)
{
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        cJSON_AddStringToObject(json_response, "error", "Failed to get WiFi status");
        return;
    }

    cJSON *wifi_info = cJSON_CreateObject();
    cJSON_AddStringToObject(wifi_info, "ssid", (const char *)ap_info.ssid);
    cJSON_AddNumberToObject(wifi_info, "rssi", ap_info.rssi);

    char bssid_str[18];
    snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
             ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);
    cJSON_AddStringToObject(wifi_info, "bssid", bssid_str);
    cJSON_AddNumberToObject(wifi_info, "vht_ch_freq1", ap_info.vht_ch_freq1);
    cJSON_AddNumberToObject(wifi_info, "vht_ch_freq2", ap_info.vht_ch_freq2);
    cJSON_AddNumberToObject(wifi_info, "channel", ap_info.primary);
    cJSON_AddNumberToObject(wifi_info, "authmode", ap_info.authmode);

    char country_cc[4] = {0};
    for (int i = 0; i < 3; ++i)
        country_cc[i] = isprint(ap_info.country.cc[i]) ? ap_info.country.cc[i] : '?';
    cJSON_AddStringToObject(wifi_info, "country", country_cc);
    cJSON_AddNumberToObject(wifi_info, "country_ies", ap_info.country.policy);
    cJSON_AddNumberToObject(wifi_info, "max_tx_power", ap_info.country.max_tx_power);

    wifi_ant_config_t ant_config;
    if (esp_wifi_get_ant(&ant_config) == ESP_OK) {
        const char *ant_mode = (ant_config.rx_ant_mode == WIFI_ANT_MODE_ANT0) ? "ANT0" :
                               (ant_config.rx_ant_mode == WIFI_ANT_MODE_ANT1) ? "ANT1" :
                               (ant_config.rx_ant_mode == WIFI_ANT_MODE_AUTO) ? "AUTO" : "UNKNOWN";
        cJSON_AddStringToObject(wifi_info, "antenna_mode", ant_mode);
    }

    wifi_bandwidth_t bw;
    const char *bw_str = "Unknown";
    if (esp_wifi_get_bandwidth(WIFI_IF_STA, &bw) == ESP_OK) {
        switch (bw) {
            case WIFI_BW20: bw_str = "20 MHz"; break;
            case WIFI_BW40: bw_str = "40 MHz"; break;
            case WIFI_BW80: bw_str = "80 MHz"; break;
            case WIFI_BW160: bw_str = "160 MHz"; break;
            case WIFI_BW80_BW80: bw_str = "80 + 80 MHz"; break;
        }
    }
    cJSON_AddStringToObject(wifi_info, "bandwidth", bw_str);
    cJSON_AddItemToObject(json_response, "wifi_status", wifi_info);
}

static void addDynamicDataInfo(cJSON *json_response)
{
    cJSON *dynamic = cJSON_CreateObject();
    cJSON_AddNumberToObject(dynamic, "free_heap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(dynamic, "min_free_heap", esp_get_minimum_free_heap_size());
    cJSON_AddNumberToObject(dynamic, "free_psram", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    cJSON_AddNumberToObject(dynamic, "uptime", esp_timer_get_time() / 1000000);
    cJSON_AddNumberToObject(dynamic, "cpu_freq_mhz", esp_clk_cpu_freq() / 1000000);
    cJSON_AddNumberToObject(dynamic, "temperature", getTemperature());

    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        cJSON_AddStringToObject(dynamic, "ssid", (const char *)ap_info.ssid);
        cJSON_AddNumberToObject(dynamic, "rssi", ap_info.rssi);
    } else {
        cJSON_AddStringToObject(dynamic, "wifi", "Failed to get WiFi AP info");
    }

    cJSON_AddItemToObject(json_response, "dynamic_data", dynamic);
}

// -------- Entry point dispatcher --------

void handleQueryForConfig(httpd_req_t *req, const char *query, bool webs)
{
    Context *ctx = Context::getInstance();
    cJSON *json_response = cJSON_CreateObject();
    if (!json_response) {
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "Failed to create JSON response object");
        if (!webs)
            httpd_resp_send_500(req);
        return;
    }

    if (strcmp(query, "sinf") == 0)
        addSystemInfo(json_response);
    else if (strcmp(query, "ws") == 0)
        addWifiStatusInfo(json_response);
    else if (strcmp(query, "ddata") == 0)
        addDynamicDataInfo(json_response);
    else {
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "Unsupported query: %s", query);
        cJSON_AddStringToObject(json_response, "error", "Unsupported query");
        if (!webs)
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unsupported query");
    }

    if (webs)
        sendJsonWebResponse(req, json_response);
    else
        sendJsonResponse(req, json_response);
}
