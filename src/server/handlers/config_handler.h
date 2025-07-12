#pragma once

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Handles a config fetch query (`cfg?q=...`) from either HTTP or WebSocket.
 *
 * @param req    HTTP request pointer.
 * @param query  Specific config key to fetch.
 * @param webs   True if the request came from WebSocket, false for HTTP GET.
 */
void handleQueryForConfig(httpd_req_t *req, const char *query, bool webs);

/**
 * @brief Handles a config update query (`cfg?cfg=...&q=...&v=...`).
 *
 * @param req    HTTP request pointer.
 * @param cfg    Configuration category (e.g., "wifi", "audio", "speech").
 * @param query  Config key to modify.
 * @param value  New value to set for the key.
 */
void handleValueForConfig(httpd_req_t *req, const char *cfg, const char *query, const char *value);

/**
 * @brief Handles HTTP GET requests to fetch config values via `?q=...`.
 *
 * URI example: /cfg?q=volume
 */
esp_err_t config_get_handler(httpd_req_t *req);

/**
 * @brief Handles HTTP GET requests to update config values via `?cfg=...&q=...&v=...`.
 *
 * URI example: /cfg?cfg=speech&q=lang&v=en
 */
esp_err_t set_config_handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif
