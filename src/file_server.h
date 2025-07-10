#pragma once
#include <string>
#include "esp_http_server.h"
#include "Arduino.h"
#include "esp_vfs_fat.h"


class FileServer
{
private:
#define UPLOAD_BASE_PATH "/spiflash/"
#define MAX_FILENAME_LEN 64
#define MAX_BUFFER_SIZE 1024
public:
    
    esp_err_t static file_upload_handler(httpd_req_t *req);
    esp_err_t static file_serve_handler(httpd_req_t *req);
    esp_err_t static delete_file_handler(httpd_req_t *req);
    bool static static_uri_match(httpd_req_t *req, const char *uri);
    
};

extern FileServer fileServer;