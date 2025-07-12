#pragma once

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief POST handler for /speech endpoint.
 */
esp_err_t speech_post_handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif
