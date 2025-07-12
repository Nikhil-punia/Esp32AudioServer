#pragma once

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Handler for WebSocket communication
 */
esp_err_t websocket_handler(httpd_req_t *req);

/**
 * @brief Handles query to fetch system web configuration
 */
void handleQueryForWebSettings(httpd_req_t *req);

#ifdef __cplusplus
}
#endif
