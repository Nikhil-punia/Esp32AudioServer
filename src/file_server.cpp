#include "file_server.h"
#include "esp_vfs_fat.h"

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif


esp_err_t FileServer::file_serve_handler(httpd_req_t *req)
{
    

    char filepath[128] = "/spiflash/";
    char query[100];
    char filename[64] = "index.html"; // Default file

    // Get query string (e.g., "file=index.html")
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {
        if (httpd_query_key_value(query, "file", filename, sizeof(filename)) != ESP_OK)
        {
            ESP_LOGW("STATIC", "No 'file' parameter, using default");
        }
    }

    // Build final path: /ffat/index.html
    strncat(filepath, filename, sizeof(filepath) - strlen(filepath) - 1);

    FILE *file = fopen(filepath, "r");
    if (!file)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }

    if (strstr(filepath, ".html"))
        httpd_resp_set_type(req, "text/html");
    else if (strstr(filepath, ".css"))
        httpd_resp_set_type(req, "text/css");
    else if (strstr(filepath, ".js"))
        httpd_resp_set_type(req, "application/javascript");
    else if (strstr(filepath, ".png"))
        httpd_resp_set_type(req, "image/png");
    else
        httpd_resp_set_type(req, "text/plain");

    char buffer[1024*4];
    size_t read_bytes;
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        if (httpd_resp_send_chunk(req, buffer, read_bytes) != ESP_OK)
        {
            fclose(file);
            return ESP_FAIL;
        }
        vTaskDelay(1);
    }
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t FileServer::file_upload_handler(httpd_req_t *req)
{
    uint64_t total = 0, used = 0;
    esp_vfs_fat_info("/spiflash", &total, &used);
    Serial.printf(" FS total: %llu, used: %llu, free: %llu\n", total, used, total - used);

    char filepath[MAX_FILENAME_LEN] = UPLOAD_BASE_PATH;
    char filename[MAX_FILENAME_LEN] = {0};
    char *buf = (char *)malloc(MAX_BUFFER_SIZE);
    if (!buf)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Buffer allocation failed");
        return ESP_FAIL;
    }

    // Parse filename from query: /upload?filename=xyz.txt
    char query[100];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {
        if (httpd_query_key_value(query, "filename", filename, sizeof(filename)) != ESP_OK)
        {
            free(buf);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Filename missing");
            return ESP_FAIL;
        }
    }
    else
    {
        free(buf);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Query missing");
        return ESP_FAIL;
    }

    // Construct full file path
    strncat(filepath, filename, sizeof(filepath) - strlen(filepath) - 1);

    // Check if file already exists
    FILE *check = fopen(filepath, "r");
    if (check)
    {
        fclose(check);
        free(buf);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File already exists");
        return ESP_FAIL;
    }

    // Open file for writing (only if it doesn't exist)
    FILE *file = fopen(filepath, "w");
    if (!file)
    {
        free(buf);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File open failed");
        return ESP_FAIL;
    }

    // Read POST body and write to file
    int received;
    int remaining = req->content_len;
    while (remaining > 0)
    {
        received = httpd_req_recv(req, buf, MIN(remaining, MAX_BUFFER_SIZE));
        if (received <= 0)
        {
            fclose(file);
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Receive failed");
            return ESP_FAIL;
        }
        size_t written = fwrite(buf, 1, received, file);
        if (written != received)
        {
            fclose(file);
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Write failed");
            return ESP_FAIL;
        }
        remaining -= received;
    }
    fflush(file);
    fsync(fileno(file));

    fclose(file);
    free(buf);
    httpd_resp_sendstr(req, "File uploaded successfully");
    return ESP_OK;
}

esp_err_t FileServer::delete_file_handler(httpd_req_t *req)
{
    char filepath[MAX_FILENAME_LEN] = UPLOAD_BASE_PATH;

    // Extract query parameter ?filename=...
    char query[100];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {
        char filename[64];
        if (httpd_query_key_value(query, "filename", filename, sizeof(filename)) == ESP_OK)
        {
            strncat(filepath, filename, sizeof(filepath) - strlen(filepath) - 1);

            // Attempt to delete the file
            if (unlink(filepath) == 0)
            {
                httpd_resp_sendstr(req, "File deleted successfully");
                return ESP_OK;
            }
            else
            {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to delete file either it does not exist or permission denied");
                return ESP_FAIL;
            }
        }
        else
        {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing 'filename' parameter");
            return ESP_FAIL;
        }
    }

    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid query");
    return ESP_FAIL;
}

bool FileServer::static_uri_match(httpd_req_t *req, const char *uri)
{
    return strncmp(req->uri, uri, strlen(uri)) == 0;
}