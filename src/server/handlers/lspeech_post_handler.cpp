#include "server/handlers/lspeech_post_handler.h"

#include "esp_log.h"
#include "cJSON.h"
#include "speech_util.h"
#include "context.h"

static const char *TAG = Context::getInstance()->TAG_HTTP_SERVER;

esp_err_t lspeech_post_handler(httpd_req_t *req)
{
    SpeechUtil::getInstance()->stopAllPreviousTasks();

    auto *args = new lSynthesizeArgs;
    Context::getInstance()->largs = args;

    if (!args)
    {
        ESP_LOGE(TAG, "Failed to allocate lSynthesizeArgs");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    args->text = std::make_unique<std::string>();
    args->voice_id = std::make_unique<std::string>();

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
        delete args;
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    cJSON *text_item = cJSON_GetObjectItem(json, "text");
    cJSON *voice_id_item = cJSON_GetObjectItem(json, "voice_id");

    if (!text_item || !cJSON_IsString(text_item))
    {
        ESP_LOGE(TAG, "Missing or invalid 'text' field");
        cJSON_Delete(json);
        delete args;
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    args->text->assign(text_item->valuestring);
    args->voice_id->assign(
        (voice_id_item && cJSON_IsString(voice_id_item)) ?
        voice_id_item->valuestring : "en-US-EmmaMultilingualNeural");

    cJSON_Delete(json);

    ESP_LOGI(TAG, "Received text: %s, voice_id: %s",
             args->text->c_str(), args->voice_id->c_str());

    BaseType_t res = xTaskCreate(
        &SpeechUtil::lspeech_synthesis_task,
        "lspeech_synth",
        4096 * 2,
        args,
        1,
        nullptr
    );

    if (res != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create lspeech_synthesis_task");
        delete args;
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "Speech synthesis started.");
    return ESP_OK;
}
