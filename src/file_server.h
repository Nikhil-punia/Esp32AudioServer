#pragma once

#include <string>
#include "esp_http_server.h"
#include "Arduino.h"
#include "esp_vfs_fat.h"

/**
 * @brief FileServer handles file-related HTTP endpoints.
 *
 * This class provides static methods to:
 * - Serve files from FFAT (`/spiflash`)
 * - Upload files via HTTP POST
 * - Delete files via HTTP DELETE (query-based)
 * 
 * All paths operate under the mount point `/spiflash`, and support basic
 * MIME type detection based on file extensions.
 */
class FileServer
{
private:
    /// Base path on FFAT (SPI Flash) for file operations
    #define UPLOAD_BASE_PATH "/spiflash/"

    /// Max allowed length for filename (including path)
    #define MAX_FILENAME_LEN 64

    /// Buffer size used for file uploads and chunked responses
    #define MAX_BUFFER_SIZE 1024

    /**
     * @brief Utility function to match URI to a static path.
     * 
     * Used to determine if a request matches a specific static path prefix.
     *
     * @param req Incoming HTTP request
     * @param uri Static URI prefix to match
     * @return true if match, false otherwise
     */
    static bool static_uri_match(httpd_req_t *req, const char *uri);

public:
    /**
     * @brief HTTP handler for uploading a file to FFAT via POST request.
     * 
     * Expected query: `?filename=your_file.txt`
     * - Receives body data and writes it to `/spiflash/your_file.txt`.
     * - Rejects if the file already exists.
     *
     * @param req HTTP POST request with file content and filename query
     * @return ESP_OK on success, ESP_FAIL on error
     */
    static esp_err_t file_upload_handler(httpd_req_t *req);

    /**
     * @brief HTTP handler for serving static files from FFAT.
     * 
     * Expected query: `?file=filename.ext`
     * - If no file is specified, defaults to `index.html`.
     * - Sets appropriate content-type based on file extension.
     *
     * @param req HTTP GET request
     * @return ESP_OK on success, ESP_FAIL on error or if file not found
     */
    static esp_err_t file_serve_handler(httpd_req_t *req);

    /**
     * @brief HTTP handler for deleting a file from FFAT.
     * 
     * Expected query: `?filename=your_file.txt`
     * - Attempts to delete the file from `/spiflash`.
     * - Returns success/failure message.
     *
     * @param req HTTP DELETE (or GET) request with filename query
     * @return ESP_OK if deleted, ESP_FAIL if not found or invalid
     */
    static esp_err_t delete_file_handler(httpd_req_t *req);

};

// Global instance of FileServer
extern FileServer fileServer;
