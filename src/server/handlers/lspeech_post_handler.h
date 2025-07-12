#pragma once

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief POST handler for /lspeech endpoint.
 * Call this directly in URI registration.
 */
esp_err_t lspeech_post_handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif
