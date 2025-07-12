#include "esp32AudioServer.h"

Esp32AudioServer::Esp32AudioServer()
    : ctx(Context::getInstance()), wifi(WifiUtil::getInstance())
{}

void Esp32AudioServer::setupServer()
{
    fsUtil.checkAndInitialObjectInFile(fsUtil.root_file_path); // Ensure config structure
    wifi->connectToBestWifiOrSoftAP(wifi->ap_ssid, wifi->ap_password);

    ServerUtil::getInstance()->start_webserver();
    AudioUtil::getInstance()->setupAudio();       // Initialize audio settings
    SpeechUtil::getInstance()->setupSpeechUtil(); // Initialize speech settings
    // audioUtil.handle_music_request("https://stream.radioparadise.com/mp3-320"); // Optional autoplay
}

void Esp32AudioServer::loopServer()
{
    wifi->wifiManagerLoop();
    AudioUtil::getInstance()->loopAudio();
}
