#include "server/handlers/root_get_handler.h"

#include "esp_log.h"
#include "cJSON.h"
#include "audio_util.h"
#include "context.h"

esp_err_t root_get_handler(httpd_req_t *req)
{
    Context *ctx = Context::getInstance();

    char query[ctx->MAX_QUERY_LEN];
    char link_url[ctx->MAX_LINK_LEN] = "";

    cJSON *json_response = cJSON_CreateObject();

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {
        ESP_LOGI(ctx->TAG_HTTP_SERVER, "Found URL query: %s", query);

        char bass_buf[16] = {0};
        char mid_buf[16] = {0};
        char tr_buf[16] = {0};
        char stop_buf[4] = {0};
        bool has_supported_query = false;

        if (!json_response)
        {
            ESP_LOGE(ctx->TAG_HTTP_SERVER, "Failed to create JSON response object");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        if (httpd_query_key_value(query, "link", link_url, sizeof(link_url)) == ESP_OK)
        {
            ESP_LOGI(ctx->TAG_HTTP_SERVER, "link=%s", link_url);
            cJSON_AddStringToObject(json_response, "link", link_url);
            AudioUtil::getInstance()->handle_music_request(link_url);
            has_supported_query = true;
        }

        if (httpd_query_key_value(query, "bass", bass_buf, sizeof(bass_buf)) == ESP_OK)
        {
            ESP_LOGI(ctx->TAG_HTTP_SERVER, "bass=%s", bass_buf);
            AudioUtil::getInstance()->setBassStr(bass_buf);
            cJSON_AddStringToObject(json_response, "bass", bass_buf);
            has_supported_query = true;
        }

        if (httpd_query_key_value(query, "mid", mid_buf, sizeof(mid_buf)) == ESP_OK)
        {
            ESP_LOGI(ctx->TAG_HTTP_SERVER, "mid=%s", mid_buf);
            AudioUtil::getInstance()->setMidStr(mid_buf);
            cJSON_AddStringToObject(json_response, "mid", mid_buf);
            has_supported_query = true;
        }

        if (httpd_query_key_value(query, "tr", tr_buf, sizeof(tr_buf)) == ESP_OK)
        {
            ESP_LOGI(ctx->TAG_HTTP_SERVER, "tr=%s", tr_buf);
            AudioUtil::getInstance()->setTrStr(tr_buf);
            cJSON_AddStringToObject(json_response, "tr", tr_buf);
            has_supported_query = true;
        }

        if (httpd_query_key_value(query, "stop", stop_buf, sizeof(stop_buf)) == ESP_OK)
        {
            ESP_LOGI(ctx->TAG_HTTP_SERVER, "stop requested");
            AudioUtil::getInstance()->stopAudio();
            cJSON_AddBoolToObject(json_response, "stop", true);
            has_supported_query = true;
        }

        AudioUtil::getInstance()->setTone(
            atoi(AudioUtil::getInstance()->getBassStr().c_str()),
            atoi(AudioUtil::getInstance()->getMidStr().c_str()),
            atoi(AudioUtil::getInstance()->getTrStr().c_str()));

        if (!has_supported_query)
        {
            cJSON_AddStringToObject(json_response, "message", "URL query Not found");
        }

        char *json_str = cJSON_Print(json_response);
        if (!json_str)
        {
            ESP_LOGE(ctx->TAG_HTTP_SERVER, "Failed to serialize JSON response");
            cJSON_Delete(json_response);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        httpd_resp_set_type(req, "application/json");
        esp_err_t res = httpd_resp_sendstr(req, json_str);
        free(json_str);
        cJSON_Delete(json_response);
        return res;
    }
    else
    {
        cJSON_AddStringToObject(json_response, "message", "No URL query found");
        char *json_str = cJSON_Print(json_response);
        if (!json_str)
        {
            ESP_LOGE(ctx->TAG_HTTP_SERVER, "Failed to serialize JSON response");
            cJSON_Delete(json_response);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        httpd_resp_set_type(req, "application/json");
        esp_err_t res = httpd_resp_sendstr(req, json_str);
        free(json_str);
        cJSON_Delete(json_response);
        return res;
    }
}
