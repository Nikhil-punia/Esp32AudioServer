#pragma once

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HTTP GET handler for the root endpoint ("/")
 *        Processes query parameters like link, bass, mid, tr, stop, etc.
 */

esp_err_t root_get_handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif
