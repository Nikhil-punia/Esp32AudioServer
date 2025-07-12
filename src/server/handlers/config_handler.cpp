#include "server/handlers/config_handler.h"
#include "esp_log.h"
#include "context.h"
#include "cJSON.h"
#include "speech_util.h"
#include "server/handlers/handler_common.h"

void handleValueForConfig(httpd_req_t *req, const char *cfg, const char *query, const char *value)
{
    Context *ctx = Context::getInstance();

    if (strcmp(cfg, "wifi") == 0)
    {
        // TODO: Handle WiFi config
    }
    else if (strcmp(cfg, "audio") == 0)
    {
        // TODO: Handle audio config
    }
    else if (strcmp(cfg, "speech") == 0)
    {
        if (SpeechUtil::getInstance()->handleSingleConfigUpdate(query, value))
        {
            
            cJSON *json_response = cJSON_CreateObject();
            if (!json_response)
            {
                ESP_LOGE(ctx->TAG_HTTP_SERVER, "Failed to create JSON response object");
                httpd_resp_send_500(req);
                return;
            }
            cJSON_AddStringToObject(json_response, "message", "Speech config updated successfully");
            sendJsonResponse(req, json_response);
        }
        else
        {
            ESP_LOGE(ctx->TAG_HTTP_SERVER, "Failed to update speech config");
            httpd_resp_send_500(req);
        }
    }
    else
    {
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "Unknown config type: %s", cfg);
        httpd_resp_send_500(req);
    }
}

esp_err_t config_get_handler(httpd_req_t *req)
{
    Context *ctx = Context::getInstance();
    char query[ctx->MAX_QUERY_LEN];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {
        ESP_LOGI(ctx->TAG_HTTP_SERVER, "Found URL query: %s", query);
        char search_query[64];
        if (httpd_query_key_value(query, "q", search_query, sizeof(search_query)) == ESP_OK)
        {
            ESP_LOGI(ctx->TAG_HTTP_SERVER, "q=%s", search_query);
            handleQueryForConfig(req, search_query, false);
            return ESP_OK;
        }
        else
        {
            ESP_LOGE(ctx->TAG_HTTP_SERVER, "'q' parameter missing");
        }
    }
    else
    {
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "No query string found");
    }

    httpd_resp_send_500(req);
    return ESP_FAIL;
}

esp_err_t set_config_handler(httpd_req_t *req)
{
    Context *ctx = Context::getInstance();
    char query[ctx->MAX_QUERY_LEN];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {
        ESP_LOGI(ctx->TAG_HTTP_SERVER, "Query: %s", query);
        char cfg[16], key[64], value[128];

        if (httpd_query_key_value(query, "cfg", cfg, sizeof(cfg)) == ESP_OK &&
            httpd_query_key_value(query, "q", key, sizeof(key)) == ESP_OK &&
            httpd_query_key_value(query, "v", value, sizeof(value)) == ESP_OK)
        {
            ESP_LOGI(ctx->TAG_HTTP_SERVER, "cfg=%s, q=%s, v=%s", cfg, key, value);
            handleValueForConfig(req, cfg, key, value);
            return ESP_OK;
        }
        else
        {
            ESP_LOGE(ctx->TAG_HTTP_SERVER, "Missing one of cfg/q/v parameters");
        }
    }
    else
    {
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "No query string found");
    }

    httpd_resp_send_500(req);
    return ESP_FAIL;
}