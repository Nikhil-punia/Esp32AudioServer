#pragma once

#include "esp_http_server.h"
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

void sendJsonResponse(httpd_req_t *req, cJSON *json_response);
void sendJsonWebResponse(httpd_req_t *req, cJSON *json_response);
void sendStringWebResponse(httpd_req_t *req, char *json_string);

void handleQueryForConfig(httpd_req_t *req, const char *query, bool webs);

#ifdef __cplusplus
}
#endif
