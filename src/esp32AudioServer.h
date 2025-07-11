#pragma once

#include "Arduino.h"
#include "Audio.h"


#include "esp_log.h"

#include "audio_util.h"
#include "speech_util.h"
#include "server/server_util.h"
#include "file_server.h"
#include "fs_util.h"
#include "context.h"
#include "wifi_util.h"

class Esp32AudioServer
{
public:
    Context *ctx;
    WifiUtil *wifi;

    Esp32AudioServer();
    void setupServer();
    void loopServer();
};