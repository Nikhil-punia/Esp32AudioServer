#include "server/handlers/websocket_handler.h"

#include "esp_log.h"
#include "cJSON.h"
#include "context.h"
#include "fs_util.h"
#include "server/handlers/handler_common.h"

void handleQueryForWebSettings(httpd_req_t *req)
{
    sendStringWebResponse(req, FsUtil::getInstance()->getBufferFromFile(fsUtil.root_file_path));
}

esp_err_t websocket_handler(httpd_req_t *req)
{
    Context *ctx = Context::getInstance();

    if (req->method == HTTP_GET)
    {
        ESP_LOGI(ctx->TAG_HTTP_SERVER, "WebSocket handshake completed");
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt = {
        .final = true,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = NULL,
        .len = 0,
    };

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK || ws_pkt.len == 0)
        return ret;

    ws_pkt.payload = (uint8_t *)malloc(ws_pkt.len + 1);
    if (!ws_pkt.payload)
        return ESP_ERR_NO_MEM;

    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret != ESP_OK)
    {
        free(ws_pkt.payload);
        return ret;
    }

    ws_pkt.payload[ws_pkt.len] = '\0';
    ESP_LOGI(ctx->TAG_HTTP_SERVER, "Received: %s", (char *)ws_pkt.payload);

    cJSON *root = cJSON_Parse((char *)ws_pkt.payload);
    if (!root)
    {
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "Invalid JSON");
        free(ws_pkt.payload);
        return ESP_FAIL;
    }

    const cJSON *name = cJSON_GetObjectItem(root, "get");
    if (name && cJSON_IsString(name))
    {
        if (strcmp(name->valuestring, "syscfg") == 0)
        {
            handleQueryForWebSettings(req);
        }
        else
        {
            handleQueryForConfig(req, name->valuestring, true);
        }
    }
    else
    {
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "Missing or invalid 'get' field in JSON");
    }

    cJSON_Delete(root);
    free(ws_pkt.payload);
    return ESP_OK;
}
