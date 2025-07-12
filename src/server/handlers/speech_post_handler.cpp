#include "server/handlers/speech_post_handler.h"

#include "esp_log.h"
#include "cJSON.h"
#include "context.h"
#include "speech_util.h"

static const char *TAG = Context::getInstance()->TAG_HTTP_SERVER;

esp_err_t speech_post_handler(httpd_req_t *req)
{
    SpeechUtil::getInstance()->stopAllPreviousTasks();

    auto *args = (SynthesizeArgs *)malloc(sizeof(SynthesizeArgs));
    Context::getInstance()->args = args;

    if (!args)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for SynthesizeArgs");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    args->text = new std::string();
    args->lang = new std::string();

    std::string req_text;
    int received_total = 0;

    while (received_total < req->content_len)
    {
        char temp[128];
        int received = httpd_req_recv(req, temp, sizeof(temp));
        if (received <= 0) break;
        req_text.append(temp, received);
        memset(temp, 0, sizeof(temp));
        received_total += received;
    }

    cJSON *json = cJSON_Parse(req_text.c_str());
    if (!json)
    {
        ESP_LOGE(TAG, "Failed to parse JSON");
        delete args->text;
        delete args->lang;
        free(args);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    cJSON *text_item = cJSON_GetObjectItem(json, "text");
    cJSON *lang_item = cJSON_GetObjectItem(json, "lang");

    if (!text_item || !cJSON_IsString(text_item))
    {
        ESP_LOGE(TAG, "Missing or invalid 'text' field");
        cJSON_Delete(json);
        delete args->text;
        delete args->lang;
        free(args);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    args->text->assign(text_item->valuestring);
    args->lang->assign(
        (lang_item && cJSON_IsString(lang_item)) ? lang_item->valuestring : "en"
    );

    cJSON_Delete(json);

    ESP_LOGI(TAG, "Received text: %s, lang: %s", args->text->c_str(), args->lang->c_str());

    BaseType_t res = xTaskCreate(
        &SpeechUtil::speech_synthesis_task,
        "speech_synth",
        4096 * 2,
        args,
        1,
        nullptr
    );

    if (res != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create speech_synthesis_task");
        delete args->text;
        delete args->lang;
        free(args);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "Speech synthesis started.");
    return ESP_OK;
}
