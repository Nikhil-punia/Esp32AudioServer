#include "server_util.h"

ServerUtil::ServerUtil(SpeechUtil *speechUtilPtr, AudioUtil *audioUtilPtr)
{
    this->speechUtil = speechUtilPtr;
    this->audioUtil = audioUtilPtr;

    websocket_uri = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = websocket_handler,
        .user_ctx = this,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL };

    websocket_uri.is_websocket = true;

    root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler,
        .user_ctx = this,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};
    speech_uri = {
        .uri = "/speech",
        .method = HTTP_POST,
        .handler = speech_post_handler,
        .user_ctx = this,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};
    lspeech_uri = {
        .uri = "/lspeech",
        .method = HTTP_POST,
        .handler = lspeech_post_handler,
        .user_ctx = this,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};

    static_file_serve = {
        .uri = "/home",
        .method = HTTP_GET,
        .handler = fileServer.file_serve_handler,
        .user_ctx = this,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};

    static_file_upload = {
        .uri = "/upload",
        .method = HTTP_POST,
        .handler = fileServer.file_upload_handler,
        .user_ctx = this,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};

    static_file_delete = {
        .uri = "/delete",
        .method = HTTP_DELETE,
        .handler = fileServer.delete_file_handler,
        .user_ctx = NULL,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};

    get_config = {
        .uri = "/cfg",
        .method = HTTP_GET,
        .handler = config_get_handler,
        .user_ctx = this,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};

    set_config = {
        .uri = "/scfg",
        .method = HTTP_GET,
        .handler = set_config_handler,
        .user_ctx = this,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};

    
}

httpd_handle_t ServerUtil::start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192 * 2 ;

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &websocket_uri);
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &speech_uri);
        httpd_register_uri_handler(server, &lspeech_uri);
        httpd_register_uri_handler(server, &static_file_upload);
        httpd_register_uri_handler(server, &static_file_serve);
        httpd_register_uri_handler(server, &static_file_delete);
        httpd_register_uri_handler(server, &get_config);
        httpd_register_uri_handler(server, &set_config);

        ESP_LOGI(TAG, "HTTP Server started on port %d", config.server_port);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to start HTTP server!");
    }

    if (!MDNS.begin("audio"))
    {
        Serial.println("Error starting mDNS");
    }
    else
    {
        Serial.println("mDNS responder started: http://audio.local/");
    }

    return server;
}

esp_err_t ServerUtil::root_get_handler(httpd_req_t *req)
{
    ServerUtil *self = static_cast<ServerUtil *>(req->user_ctx);
    if (!self)
        return ESP_FAIL;

    char query[self->MAX_QUERY_LEN];
    char link_url[self->MAX_LINK_LEN] = "";

    cJSON *json_response = cJSON_CreateObject();

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {

        ESP_LOGI(self->TAG, "Found URL query: %s", query);

        char bass_buf[16] = {0};
        char mid_buf[16] = {0};
        char tr_buf[16] = {0};

        bool has_supported_query = false;

        // json response for the params
        if (!json_response)
        {
            ESP_LOGE(self->TAG, "Failed to create JSON response object");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        if (httpd_query_key_value(query, "link", link_url, sizeof(link_url)) == ESP_OK)
        {
            ESP_LOGI(self->TAG, "Found URL query parameter => link=%s", link_url);
            cJSON_AddStringToObject(json_response, "link", link_url);
            has_supported_query = true;
            self->stopAllPreviousTasks(self);
            (*self->audioUtil).handle_music_request(link_url);
        }

        if (httpd_query_key_value(query, "bass", bass_buf, sizeof(bass_buf)) == ESP_OK)
        {
            ESP_LOGI(self->TAG, "Found URL query parameter => bass=%s", bass_buf);
            (*self->audioUtil).setBassStr(bass_buf);
            has_supported_query = true;
            cJSON_AddStringToObject(json_response, "bass", bass_buf);
        }

        if (httpd_query_key_value(query, "mid", mid_buf, sizeof(mid_buf)) == ESP_OK)
        {
            ESP_LOGI(self->TAG, "Found URL query parameter => mid=%s", mid_buf);
            (*self->audioUtil).setMidStr(mid_buf);
            has_supported_query = true;
            cJSON_AddStringToObject(json_response, "mid", mid_buf);
        }

        if (httpd_query_key_value(query, "tr", tr_buf, sizeof(tr_buf)) == ESP_OK)
        {
            ESP_LOGI(self->TAG, "Found URL query parameter => tr=%s", tr_buf);
            (*self->audioUtil).setTrStr(tr_buf);
            has_supported_query = true;
            cJSON_AddStringToObject(json_response, "tr", tr_buf);
        }

        char stop_buf[4] = {0};
        if (httpd_query_key_value(query, "stop", stop_buf, sizeof(stop_buf)) == ESP_OK)
        {
            ESP_LOGI(self->TAG, "Found URL query parameter => stop");
            (*self->audioUtil).stopAudio();
            has_supported_query = true;
            cJSON_AddBoolToObject(json_response, "stop", 1); // 1 = true
        }

        self->audioUtil->setTone(
            atoi((*self->audioUtil).getBassStr().c_str()),
            atoi((*self->audioUtil).getMidStr().c_str()),
            atoi((*self->audioUtil).getTrStr().c_str()));

        if (!has_supported_query)
        {
            cJSON_AddStringToObject(json_response, "message", "URL query Not found");
        }

        // send the JSON response
        char *json_str = cJSON_Print(json_response);
        if (!json_str)
        {
            ESP_LOGE(self->TAG, "Failed to serialize JSON response");
            cJSON_Delete(json_response);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        httpd_resp_set_type(req, "application/json");
        esp_err_t res = httpd_resp_sendstr(req, json_str);
        free(json_str);
        cJSON_Delete(json_response);
        if (res != ESP_OK)
        {
            ESP_LOGE(self->TAG, "Failed to send response");
            return res;
        }
        ESP_LOGI(self->TAG, "Response sent successfully");
        return ESP_OK;
    }
    else
    {
        // No URL query found
        cJSON_AddStringToObject(json_response, "message", "No URL query found");
        char *json_str = cJSON_Print(json_response);
        if (!json_str)
        {
            ESP_LOGE(self->TAG, "Failed to serialize JSON response");
            cJSON_Delete(json_response);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        httpd_resp_set_type(req, "application/json");
        esp_err_t res = httpd_resp_sendstr(req, json_str);
        free(json_str);
        cJSON_Delete(json_response);
        if (res != ESP_OK)
        {
            ESP_LOGE(self->TAG, "Failed to send response");
            return res;
        }
        ESP_LOGI(self->TAG, "Response sent successfully with no query");
        return ESP_FAIL;
    };
};

esp_err_t ServerUtil::lspeech_post_handler(httpd_req_t *req)
{
    ServerUtil *self = static_cast<ServerUtil *>(req->user_ctx);
    if (!self)
        return ESP_FAIL;

    // Stop any previous speech tasks
    self->stopAllPreviousTasks(self);

    auto *args = new lSynthesizeArgs; // Use new, not malloc!

    // set the pointer of args to the largs of ServerUtil
    self->largs = args;

    if (!self->largs)
    {
        Serial.println("Failed to allocate memory for SynthesizeArgs");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    self->largs->speechUtil = self->speechUtil;
    self->largs->audioUtil = self->audioUtil;
    self->largs->text = std::make_unique<std::string>();
    self->largs->voice_id = std::make_unique<std::string>();

    std::string req_text;
    int received_total = 0;
    while (received_total < req->content_len)
    {
        char temp[128];
        int received = httpd_req_recv(req, temp, sizeof(temp));
        if (received <= 0)
            break;
        req_text.append(temp, received);
        // free the temp buffer
        memset(temp, 0, sizeof(temp)); // Clear the buffer
        received_total += received;
    }

    cJSON *json = cJSON_Parse(req_text.c_str());

    if (json == NULL)
    {
        Serial.println("Failed to parse JSON");
        httpd_resp_send_500(req);
        delete self->largs;
        self->largs = nullptr; // Prevent double-free
        return ESP_FAIL;
    }
    cJSON *text_item = cJSON_GetObjectItem(json, "text");
    cJSON *voice_id_item = cJSON_GetObjectItem(json, "voice_id");

    if (text_item == NULL || !cJSON_IsString(text_item))
    {
        Serial.println("Invalid JSON format");
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        delete self->largs;
        self->largs = nullptr; // Prevent double-free
        return ESP_FAIL;
    }

    self->largs->text->assign(text_item->valuestring);
    self->largs->voice_id->assign(
        (voice_id_item && cJSON_IsString(voice_id_item)) ? voice_id_item->valuestring : "en-US-EmmaMultilingualNeural");

    cJSON_Delete(json);
    Serial.printf("Received text: %s, voice_id: %s\n", self->largs->text->c_str(), self->largs->voice_id->c_str());
    BaseType_t res = xTaskCreate(&SpeechUtil::lspeech_synthesis_task, "lspeech_synth", 4096 * 2, self->largs, 1, NULL);

    if (res != pdPASS)
    {
        Serial.print("Failed to create lspeech_synthesis_task");
        delete self->largs;
        self->largs = nullptr; // Prevent double-free
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    else
    {
        Serial.print("lspeech_synthesis_task created successfully");
    }

    httpd_resp_sendstr(req, "Speech synthesis started.");
    return ESP_OK;
}

esp_err_t ServerUtil::speech_post_handler(httpd_req_t *req)
{
    ServerUtil *self = static_cast<ServerUtil *>(req->user_ctx);
    if (!self)
        return ESP_FAIL;

    // Stop any previous speech tasks
    self->stopAllPreviousTasks(self);

    auto *args = (SynthesizeArgs *)malloc(sizeof(SynthesizeArgs));

    // set the pointer of args to the largs of ServerUtil
    self->args = args;

    if (!self->args)
    {
        Serial.println("Failed to allocate memory for SynthesizeArgs");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    self->args->speechUtil = self->speechUtil;
    self->args->audioUtil = self->audioUtil;
    self->args->text = new std::string();
    self->args->lang = new std::string();

    std::string req_text;
    int received_total = 0;
    while (received_total < req->content_len)
    {
        char temp[128];
        int received = httpd_req_recv(req, temp, sizeof(temp));
        if (received <= 0)
            break;
        req_text.append(temp, received);
        // delete the temp buffer
        memset(temp, 0, sizeof(temp)); // Clear the buffer
        received_total += received;
    }

    cJSON *json = cJSON_Parse(req_text.c_str());
    if (json == NULL)
    {
        Serial.println("Failed to parse JSON");
        httpd_resp_send_500(req);
        delete self->args->text;
        delete self->args->lang;
        free(self->args);
        self->args = nullptr; // Prevent double-free
        return ESP_FAIL;
    }
    cJSON *text_item = cJSON_GetObjectItem(json, "text");
    cJSON *voice_id_item = cJSON_GetObjectItem(json, "lang");

    if (text_item == NULL || !cJSON_IsString(text_item))
    {
        Serial.println("Invalid JSON format");
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        delete self->args->text;
        delete self->args->lang;
        free(self->args);
        self->args = nullptr; // Prevent double-free
        return ESP_FAIL;
    }

    self->args->text->assign(text_item->valuestring);
    self->args->lang->assign(
        (voice_id_item && cJSON_IsString(voice_id_item)) ? voice_id_item->valuestring : "en");

    cJSON_Delete(json);
    Serial.printf("Received text: %s, voice_id: %s\n", self->args->text->c_str(), self->args->lang->c_str());
    BaseType_t res = xTaskCreate(&SpeechUtil::speech_synthesis_task, "speech_synth", 4096 * 2, self->args, 1, NULL);

    if (res != pdPASS)
    {
        Serial.print("Failed to create speech_synthesis_task");
        delete self->args->text;
        delete self->args->lang;
        free(self->args);
        self->args = nullptr; // Prevent double-free
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    else
    {
        Serial.print("speech_synthesis_task created successfully");
    }

    httpd_resp_sendstr(req, "Speech synthesis started.");
    return ESP_OK;
}

bool ServerUtil::stopAllPreviousTasks(ServerUtil *self)
{
    // Stop tasks first
    if (self->speechUtil->speech_task_running())
    {
        self->speechUtil->stop_speech_task();
        if (self->args != nullptr)
        {
            delete self->args->text;
            free(self->args);
            self->args = nullptr;
        }
    }

    if (self->speechUtil->speech_ltask_running())
    {
        self->speechUtil->stop_lspeech_task();
        if (self->largs != nullptr)
        {
            delete self->largs;
            self->largs = nullptr;
        }
    }

    self->audioUtil->stopAudio();
    return true;
}

esp_err_t ServerUtil::config_get_handler(httpd_req_t *req)
{

    ServerUtil *self = static_cast<ServerUtil *>(req->user_ctx);
    if (!self)
        return ESP_FAIL;

    // there is a query parameter in the URL &q=... which is the query to search for
    char query[self->MAX_QUERY_LEN];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {
        ESP_LOGI(self->TAG, "Found URL query: %s", query);
        char search_query[64];
        if (httpd_query_key_value(query, "q", search_query, sizeof(search_query)) == ESP_OK)
        {
            ESP_LOGI(self->TAG, "Found URL query parameter => q=%s", search_query);
            // Handle the query for config
            handleQueryForConfig(req, self, search_query, false);
            return ESP_OK;
        }
        else
        {
            ESP_LOGE(self->TAG, "Query parameter 'q' not found");
            httpd_resp_send_500(req); // Bad Request
            return ESP_FAIL;
        }
    }
    else
    {
        ESP_LOGE(self->TAG, "No URL query found");
        httpd_resp_send_500(req); // Bad Request
        return ESP_FAIL;
    }
}

esp_err_t ServerUtil::set_config_handler(httpd_req_t *req)
{
    ServerUtil *self = static_cast<ServerUtil *>(req->user_ctx);
    if (!self)
        return ESP_FAIL;

    char query[self->MAX_QUERY_LEN];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {
        ESP_LOGI(self->TAG, "Found URL query: %s", query);
        char config_type[16];
        char config_query[64];
        char config_value[128];
        if (httpd_query_key_value(query, "cfg", config_type, sizeof(config_type)) == ESP_OK)
        {
            ESP_LOGI(self->TAG, "Found URLconfig parameter => cfg=%s", config_type);

            if (httpd_query_key_value(query, "q", config_query, sizeof(config_query)) == ESP_OK)
            {
                ESP_LOGI(self->TAG, "Found URL query parameter => q=%s", config_query);
                if (httpd_query_key_value(query, "v", config_value, sizeof(config_value)) == ESP_OK)
                {
                    ESP_LOGI(self->TAG, "Found URL query parameter => v=%s", config_value);
                    handleValueForConfig(req, self, config_type, config_query, config_value);
                }
                else
                {
                    ESP_LOGE(self->TAG, "Query parameter 'v' not found");
                    httpd_resp_send_500(req); // Bad Request
                }
            }
            else
            {
                ESP_LOGE(self->TAG, "Query parameter 'q' not found");
                httpd_resp_send_500(req); // Bad Request
            }

            return ESP_OK;
        }
        else
        {
            ESP_LOGE(self->TAG, "Query parameter 'cfg' not found");
            httpd_resp_send_500(req); // Bad Request
            return ESP_FAIL;
        }
    }
    else
    {
        ESP_LOGE(self->TAG, "No URL query found");
        httpd_resp_send_500(req); // Bad Request
        return ESP_FAIL;
    }
}

void ServerUtil::handleValueForConfig(httpd_req_t *req, ServerUtil *self, const char *cfg, const char *query, const char *value)
{
            if (strcmp(cfg, "wifi") == 0)
            {
            }
            else if (strcmp(cfg, "audio") == 0)
            {
            }
            else if (strcmp(cfg, "speech") == 0)
            {
                if (self->speechUtil->handleSingleConfigUpdate(query, value))
                {
                    cJSON *json_response = cJSON_CreateObject();
                    if (!json_response)
                    {
                        ESP_LOGE(self->TAG, "Failed to create JSON response object");
                        httpd_resp_send_500(req);
                        return;
                    }
                    cJSON_AddStringToObject(json_response, "message", "Speech config updated successfully");
                    sendJsonResponse(req, json_response, self);
                }
                else
                {
                    ESP_LOGE(self->TAG, "Failed to update speech config");
                    httpd_resp_send_500(req); // Bad Request
                }
            }else{
                ESP_LOGE(self->TAG, "Unknown config type: %s", cfg);
                httpd_resp_send_500(req); // Bad Request
            }
}

void ServerUtil::handleQueryForConfig(httpd_req_t *req, ServerUtil *self, const char *query, bool webs)
{
    cJSON *json_response = cJSON_CreateObject();
    if (!json_response)
    {
        ESP_LOGE(self->TAG, "Failed to create JSON response object");
        if (!webs)
        {
            httpd_resp_send_500(req);
        }
        return;
    }

    // Handle various queries starting with system information
    if (strcmp(query, "sinf") == 0)
    {
        cJSON_AddNumberToObject(json_response, "uptime", esp_timer_get_time() / 1000000); // Uptime in seconds
        cJSON_AddStringToObject(json_response, "version", "1.0.0");                       // Replace with actual version
        cJSON_AddStringToObject(json_response, "build_time", __DATE__ " " __TIME__);      // Build time
        cJSON_AddStringToObject(json_response, "sdk_version", esp_get_idf_version());     // IDF version
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);
        const char *chip_model_str = "Unknown";
        switch (chip_info.model)
        {
        case CHIP_ESP32:
            chip_model_str = "ESP32";
            break;
        case CHIP_ESP32S2:
            chip_model_str = "ESP32-S2";
            break;
        case CHIP_ESP32S3:
            chip_model_str = "ESP32-S3";
            break;
        case CHIP_ESP32C3:
            chip_model_str = "ESP32-C3";
            break;
        case CHIP_ESP32H2:
            chip_model_str = "ESP32-H2";
            break;
        case CHIP_ESP32C2:
            chip_model_str = "ESP32-C2";
            break;
        case CHIP_ESP32C6:
            chip_model_str = "ESP32-C6";
            break;
        default:
            chip_model_str = "Unknown";
            break;
        }
        cJSON_AddStringToObject(json_response, "chip_model", chip_model_str);        // Chip model
        cJSON_AddNumberToObject(json_response, "chip_revision", chip_info.revision); // Chip revision
        cJSON_AddNumberToObject(json_response, "cores", chip_info.cores);            // Number of cores
        int cpu_freq_mhz = esp_clk_cpu_freq() / 1000000;                             // Convert to MHz
        cJSON_AddNumberToObject(json_response, "cpu_freq_mhz", cpu_freq_mhz);        // CPU frequency in MHz
        // cpu temperature
        cJSON_AddNumberToObject(json_response, "temperature", ServerUtil::getTemperature()); // Temperature in Celsius
        cJSON_AddNumberToObject(json_response, "features", chip_info.features);              // Chip features
        size_t free_heap = esp_get_free_heap_size();
        cJSON_AddNumberToObject(json_response, "free_heap", free_heap);
        size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
        cJSON_AddNumberToObject(json_response, "total_heap", total_heap);
        size_t min_free_heap = esp_get_minimum_free_heap_size();
        cJSON_AddNumberToObject(json_response, "min_free_heap", min_free_heap);
        size_t psram_size = esp_psram_get_size();
        cJSON_AddNumberToObject(json_response, "psram_size", psram_size);
        size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        cJSON_AddNumberToObject(json_response, "free_psram", free_psram);
        

        // Get flash size
        esp_flash_t *flash = esp_flash_default_chip;
        if (flash)
        {
            uint32_t flash_size = 0;
            if (esp_flash_get_size(flash, &flash_size) == ESP_OK)
            {
                cJSON_AddNumberToObject(json_response, "flash_size", flash_size);
            }
            else
            {
                cJSON_AddStringToObject(json_response, "flash_size", "Unknown");
            }
        }
        else
        {
            cJSON_AddStringToObject(json_response, "flash_size", "Unknown");
        }
        cJSON_AddStringToObject(json_response, "board", CONFIG_IDF_TARGET); // Board name from Kconfig
        cJSON_AddStringToObject(json_response, "chip", CONFIG_IDF_TARGET);  // Chip name from Kconfig
    }
    // get wifi status
    else if (strcmp(query, "ws") == 0)
    {
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
        {
            cJSON *wifi_info = cJSON_CreateObject();
            cJSON_AddStringToObject(wifi_info, "ssid", (const char *)ap_info.ssid);
            cJSON_AddNumberToObject(wifi_info, "rssi", ap_info.rssi);
            // Format BSSID as MAC address string
            char bssid_str[18];
            snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                     ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
                     ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);
            cJSON_AddStringToObject(wifi_info, "bssid", bssid_str);
            cJSON_AddNumberToObject(wifi_info, "vht_ch_freq1", ap_info.vht_ch_freq1);
            cJSON_AddNumberToObject(wifi_info, "vht_ch_freq2", ap_info.vht_ch_freq2);
            cJSON_AddNumberToObject(wifi_info, "channel", ap_info.primary);
            cJSON_AddNumberToObject(wifi_info, "authmode", ap_info.authmode);
            // ap_info.country.cc may contain non-printable characters, so sanitize it
            char country_cc[4] = {0};
            for (int i = 0; i < 3; ++i)
            {
                if (isprint((unsigned char)ap_info.country.cc[i]))
                    country_cc[i] = ap_info.country.cc[i];
                else
                    country_cc[i] = '?';
            }
            cJSON_AddStringToObject(wifi_info, "country", country_cc);
            cJSON_AddNumberToObject(wifi_info, "country_ies", ap_info.country.policy);
            cJSON_AddNumberToObject(wifi_info, "max_tx_power", ap_info.country.max_tx_power);
            // wifi_ant_config_t is a struct that contains antenna configuration information
            wifi_ant_config_t ant_config;
            if (esp_wifi_get_ant(&ant_config) == ESP_OK)
            {
                // Use the rx_ant_mode or other fields as needed
                cJSON_AddStringToObject(wifi_info, "antenna_mode", (ant_config.rx_ant_mode == WIFI_ANT_MODE_ANT0) ? "ANT0" : (ant_config.rx_ant_mode == WIFI_ANT_MODE_ANT1) ? "ANT1"
                                                                                                                         : (ant_config.rx_ant_mode == WIFI_ANT_MODE_AUTO)   ? "AUTO"
                                                                                                                                                                            : "UNKNOWN");
            }
            else
            {
                cJSON_AddStringToObject(wifi_info, "antenna_mode", "Unknown");
            }
            wifi_bandwidth_t bw;
            if (esp_wifi_get_bandwidth(WIFI_IF_STA, &bw) == ESP_OK)
            {
                switch (bw)
                {
#if defined(WIFI_BW_HT20) && (WIFI_BW_HT20 != WIFI_BW20)
                case WIFI_BW_HT20:
#endif
                case WIFI_BW20:
                    cJSON_AddStringToObject(wifi_info, "bandwidth", "20 MHz");
                    break;
#if defined(WIFI_BW_HT40)
                case WIFI_BW_HT40:
#else
                case WIFI_BW40:
#endif
                    cJSON_AddStringToObject(wifi_info, "bandwidth", "40 MHz");
                    break;
                case WIFI_BW80:
                    cJSON_AddStringToObject(wifi_info, "bandwidth", "80 MHz");
                    break;
                case WIFI_BW160:
                    cJSON_AddStringToObject(wifi_info, "bandwidth", "160 MHz");
                    break;
                case WIFI_BW80_BW80:
                    cJSON_AddStringToObject(wifi_info, "bandwidth", "80 + 80 MHz");
                    break;
                default:
                    cJSON_AddStringToObject(wifi_info, "bandwidth", "Unknown");
                    break;
                }
            }
            else
            {
                cJSON_AddStringToObject(wifi_info, "bandwidth", "Unknown");
            }

            cJSON_AddItemToObject(json_response, "wifi_status", wifi_info);
        }
        else
        {
            cJSON_AddStringToObject(json_response, "error", "Failed to get WiFi status");
        }
    }
    else if (strcmp(query, "ddata") == 0)
    {
        // dynamic data handler
        wifi_ap_record_t ap_info;
        cJSON *dynamic_data = cJSON_CreateObject();
        size_t free_heap = esp_get_free_heap_size();
        cJSON_AddNumberToObject(dynamic_data, "free_heap", free_heap);
        size_t min_free_heap = esp_get_minimum_free_heap_size();
        cJSON_AddNumberToObject(dynamic_data, "min_free_heap", min_free_heap);
        size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        cJSON_AddNumberToObject(dynamic_data, "free_psram", free_psram);
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
        {
            cJSON_AddStringToObject(dynamic_data, "ssid", (const char *)ap_info.ssid);
            cJSON_AddNumberToObject(dynamic_data, "rssi", ap_info.rssi);
        }
        else
        {
            cJSON_AddStringToObject(dynamic_data, "wifi", "Failed to get WiFi AP info");
        }
        cJSON_AddNumberToObject(dynamic_data, "uptime", esp_timer_get_time() / 1000000); // Uptime in seconds
        // cpu frequency
        int cpu_freq_mhz = esp_clk_cpu_freq() / 1000000;                     // Convert to MHz
        cJSON_AddNumberToObject(dynamic_data, "cpu_freq_mhz", cpu_freq_mhz); // CPU frequency in MHz
        // cpu temperature
        cJSON_AddNumberToObject(dynamic_data, "temperature", ServerUtil::getTemperature()); // Temperature in Celsius

        cJSON_AddItemToObject(json_response, "dynamic_data", dynamic_data);
    }

    else
    {
        ESP_LOGE(self->TAG, "Unsupported query: %s", query);
        cJSON_AddStringToObject(json_response, "error", "Unsupported query");
    }

    // Send the JSON response

    if (webs)
    {
        sendJsonWebResponse(req, json_response, self);
    }
    else
    {
        sendJsonResponse(req, json_response, self);
    }
}

void ServerUtil::sendJsonResponse(httpd_req_t *req, cJSON *json_response, ServerUtil *self)
{
    if (!json_response)
    {
        ESP_LOGE(self->TAG, "JSON response is NULL");
        httpd_resp_send_500(req);
        return;
    }

    char *json_str = cJSON_Print(json_response);
    if (!json_str)
    {
        ESP_LOGE(self->TAG, "Failed to serialize JSON response");
        cJSON_Delete(json_response);
        httpd_resp_send_500(req);
        return;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    esp_err_t res = httpd_resp_sendstr(req, json_str);
    free(json_str);
    cJSON_Delete(json_response);

    if (res != ESP_OK)
    {
        ESP_LOGE(self->TAG, "Failed to send response");
    }
}

void ServerUtil::sendJsonWebResponse(httpd_req_t *req, cJSON *json_response, ServerUtil *self)
{
    if (!json_response)
    {
        ESP_LOGE(self->TAG, "JSON response is NULL");
        return;
    }

    char *json_str = cJSON_PrintUnformatted(json_response);
    if (!json_str)
    {
        ESP_LOGE(self->TAG, "Failed to serialize JSON response");
        cJSON_Delete(json_response);
        return;
    }

    httpd_ws_frame_t ws_res = {
        .final = true,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)json_str,
        .len = strlen(json_str),
    };

    esp_err_t res = httpd_ws_send_frame(req, &ws_res);
    free(json_str);
    cJSON_Delete(json_response);

    if (res != ESP_OK)
    {
        ESP_LOGE(self->TAG, "Failed to send websocket response");
    }
}

void ServerUtil::sendStringWebResponse(httpd_req_t *req, char *json_string, ServerUtil *self)
{
    if (!json_string)
    {
        ESP_LOGE(self->TAG, "JSON response is NULL");
        return;
    }

    httpd_ws_frame_t ws_res = {
        .final = true,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)json_string,
        .len = strlen(json_string),
    };

    esp_err_t res = httpd_ws_send_frame(req, &ws_res);
    free(json_string);

    if (res != ESP_OK)
    {
        ESP_LOGE(self->TAG, "Failed to send websocket response");
    }
}

static temperature_sensor_handle_t temp_sensor = NULL;
static bool temp_sensor_enabled = false;

float ServerUtil::getTemperature()
{

    if (temp_sensor == NULL)
    {
        temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 100);
        esp_err_t err = temperature_sensor_install(&temp_sensor_config, &temp_sensor);
        if (err != ESP_OK)
        {
            return -1;
        }
    }

    if (!temp_sensor_enabled)
    {
        if (temperature_sensor_enable(temp_sensor) == ESP_OK)
        {
            temp_sensor_enabled = true;
        }
        else
        {
            return -1;
        }
    }

    float tsens_out;
    if (temperature_sensor_get_celsius(temp_sensor, &tsens_out) == ESP_OK)
    {
        return tsens_out;
    }
    return -1; // Return -1 if unable to read temperature
}

esp_err_t ServerUtil::websocket_handler(httpd_req_t *req)
{
    ServerUtil *self = static_cast<ServerUtil *>(req->user_ctx);
    if (!self)
        return ESP_FAIL;

    if (req->method == HTTP_GET)
    {
        ESP_LOGI(self->TAG, "WebSocket handshake completed");
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt = {
        .final = true,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = NULL,
        .len = 0,
    };

    // Get payload length
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK || ws_pkt.len == 0)
        return ret;

    ws_pkt.payload = (uint8_t *)malloc(ws_pkt.len + 1);
    if (!ws_pkt.payload)
        return ESP_ERR_NO_MEM;

    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret != ESP_OK)
    {
        free(ws_pkt.payload);
        return ret;
    }

    ws_pkt.payload[ws_pkt.len] = '\0';
    ESP_LOGI(self->TAG, "Received: %s", (char *)ws_pkt.payload);

    // Parse JSON
    cJSON *root = cJSON_Parse((char *)ws_pkt.payload);
    if (!root)
    {
        ESP_LOGE(self->TAG, "Invalid JSON");
        free(ws_pkt.payload);
        return ESP_FAIL;
    }

    const cJSON *name = cJSON_GetObjectItem(root, "get");
    if (name && cJSON_IsString(name))
    {
        if (strcmp(name->valuestring, "syscfg") == 0)
        {
            handleQueryForWebSettings(req, self);
        }
        else
        {
            handleQueryForConfig(req, self, name->valuestring, true);
        }
    }
    else
    {
        ESP_LOGE(self->TAG, "Missing or invalid 'get' field in JSON");
    }

    // Clean up
    cJSON_Delete(root);
    free(ws_pkt.payload);
    return ESP_OK;
}

void ServerUtil::handleQueryForWebSettings(httpd_req_t *req, ServerUtil *self)
{
    self->sendStringWebResponse(req, fsUtil.getBufferFromFile(fsUtil.root_file_path), self);
}