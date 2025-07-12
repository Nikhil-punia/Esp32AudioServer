#pragma once

/**
 * @file server_util.h
 * @brief Declares the ServerUtil class responsible for starting and managing the HTTP server,
 *        registering URI handlers, and providing a singleton interface to access the server utility.
 */

#include <ESPmDNS.h>
#include <string>
#include <vector>

#include "freertos/semphr.h"
#include "esp_http_server.h"
#include <cJSON.h>

#include "file_server.h"
#include "audio_util.h"
#include "speech_util.h"

#include "context.h"
#include "server/handlers/root_get_handler.h"
#include "server/handlers/lspeech_post_handler.h"
#include "server/handlers/speech_post_handler.h"
#include "server/handlers/websocket_handler.h"
#include "server/handlers/config_handler.h"

/**
 * @class ServerUtil
 * @brief Singleton class that encapsulates initialization and operation of the HTTP server.
 * 
 * This class handles:
 * - Web server start-up
 * - URI registration for various HTTP endpoints
 * - Delegation to appropriate request handlers (root, speech, file uploads, etc.)
 */
class ServerUtil
{
private:
    /**
     * @brief Private constructor for singleton pattern.
     */
    ServerUtil();

    // Disable copy constructor and assignment
    ServerUtil(const ServerUtil&) = delete;
    ServerUtil& operator=(const ServerUtil&) = delete;

    /**
     * @brief Context instance providing global config and constants.
     */
    Context *ctx ;

    /**
     * @brief URI handlers for different endpoints.These are defined in server/handlers/ files.
     *        and are defined and registered in server_util.cpp.
     */
    httpd_uri_t root_uri;
    httpd_uri_t speech_uri;
    httpd_uri_t lspeech_uri;
    httpd_uri_t static_file_upload;
    httpd_uri_t static_file_serve;
    httpd_uri_t static_file_delete;
    httpd_uri_t get_config;
    httpd_uri_t set_config;
    httpd_uri_t websocket_uri;

public:
    /**
     * @brief Accessor for the singleton instance of ServerUtil.
     * @return Pointer to the singleton ServerUtil instance.
     */
    static ServerUtil* getInstance();

    /**
     * @brief Starts the HTTP server and registers all URI handlers.
     * 
     * @return httpd_handle_t Handle to the running HTTP server instance.
     */
    httpd_handle_t start_webserver();
};
