#include "server_util.h"

ServerUtil *ServerUtil::getInstance()
{
    static ServerUtil instance;
    return &instance;
}

ServerUtil::ServerUtil():ctx(Context::getInstance())
{
    websocket_uri = {
        .uri = ctx->http_uri_websocket,
        .method = HTTP_GET,
        .handler = websocket_handler,
        .user_ctx = this,
        .is_websocket = true,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};

    websocket_uri.is_websocket = true;

    root_uri = {
        .uri = ctx->http_uri_root,
        .method = HTTP_GET,
        .handler = root_get_handler,
        .user_ctx = this,
        .is_websocket = false,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};

    speech_uri = {
        .uri = ctx->http_uri_speech,
        .method = HTTP_POST,
        .handler = speech_post_handler,
        .user_ctx = this,
        .is_websocket = false,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};

    lspeech_uri = {
        .uri = ctx->http_uri_lspeech,
        .method = HTTP_POST,
        .handler = lspeech_post_handler,
        .user_ctx = this,
        .is_websocket = false,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};

    static_file_serve = {
        .uri = ctx->http_uri_static_file_serve,
        .method = HTTP_GET,
        .handler = fileServer.file_serve_handler,
        .user_ctx = this,
        .is_websocket = false,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};

    static_file_upload = {
        .uri = ctx->http_uri_static_file_upload,
        .method = HTTP_POST,
        .handler = fileServer.file_upload_handler,
        .user_ctx = this,
        .is_websocket = false,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};

    static_file_delete = {
        .uri = ctx->http_uri_static_file_delete,
        .method = HTTP_DELETE,
        .handler = fileServer.delete_file_handler,
        .user_ctx = NULL,
        .is_websocket = false,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};

    get_config = {
        .uri = ctx->http_uri_get_config,
        .method = HTTP_GET,
        .handler = config_get_handler,
        .user_ctx = this,
        .is_websocket = false,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};

    set_config = {
        .uri = ctx->http_uri_set_config,
        .method = HTTP_GET,
        .handler = set_config_handler,
        .user_ctx = this,
        .is_websocket = false,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL};
}

httpd_handle_t ServerUtil::start_webserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = ctx->http_stack_size;
    config.max_uri_handlers = 20; // Adjust as needed

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

        ESP_LOGI(ctx->TAG_HTTP_SERVER, "HTTP Server started on port %d", config.server_port);
    }
    else
    {
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "Failed to start HTTP server!");
    }

    // setup for Mdns

    if (!MDNS.begin(ctx->mdns_service_name))
    {
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "Error setting up MDNS responder!");
    }
    else if (!MDNS.addService("http", "tcp", 80))
    {
        ESP_LOGE(ctx->TAG_HTTP_SERVER, "Error adding HTTP service to mDNS");
    }
    else
    {
        std::string port_str = std::to_string(config.server_port);

        MDNS.addServiceTxt("http", "tcp", "path", ctx->http_uri_static_file_serve);
        MDNS.addServiceTxt("http", "tcp", "port", port_str.c_str());
        MDNS.addServiceTxt("http", "tcp", "version", "1.0");
        MDNS.addServiceTxt("http", "tcp", "description", "ESP32 Audio Server");
        MDNS.addServiceTxt("http", "tcp", "host", (std::string(ctx->mdns_service_name) + ".local").c_str());
        MDNS.addServiceTxt("http", "tcp", "status", "running");
        MDNS.addServiceTxt("http", "tcp", "author", "Esp32AudioServer");
        MDNS.addServiceTxt("http", "tcp", "location", "Local Network");
        MDNS.addServiceTxt("http", "tcp", "license", "MIT");

        ESP_LOGI(ctx->TAG_HTTP_SERVER, "mDNS responder started: %s.local", ctx->mdns_service_name);
    }

    return server;
}
