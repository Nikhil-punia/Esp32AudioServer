#pragma once
#include "Audio.h"
#include <cstring> // instead of <string.h>
#include <string>
#include "esp_log.h"
#include "cJSON.h"
#include "fs_util.h"

// Define I2S pins based on the target ESP32 board
#ifdef CONFIG_IDF_TARGET_ESP32
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S3
#define I2S_DOUT 40
#define I2S_BCLK 42
#define I2S_LRC 41
#endif

#ifdef CONFIG_IDF_TARGET_ESP32P4
#define I2S_DOUT 22
#define I2S_BCLK 20
#define I2S_LRC 21
#endif

class AudioUtil
{
public:
    static AudioUtil *getInstance(); // Singleton accessor

    void stopAudio();
    void setBassStr(const std::string &bass) { bass_str = bass; };
    void setMidStr(const std::string &mid) { mid_str = mid; };
    void setTrStr(const std::string &tr) { tr_str = tr; };
    Audio &getAudio() { return audio; };
    const std::string &getBassStr() const { return bass_str; };
    const std::string &getMidStr() const { return mid_str; };
    const std::string &getTrStr() const { return tr_str; };
    void setStreamUrl(const std::string &url) { next_stream_url = url; };
    const std::string &getStreamUrl() const { return next_stream_url; };
    void setupAudio();
    void handle_music_request(const char *url);
    void handle_google_tts(const char *text, const char *lang);
    void handle_local_tts(std::string text, std::string voice_id,std::string host,int port,std::string path);
    void setTone(int b, int m, int t);
    void loopAudio();

private:
    AudioUtil();                                      // Private constructor
    AudioUtil(const AudioUtil &) = delete;            // Prevent copy
    AudioUtil &operator=(const AudioUtil &) = delete; // Prevent assignment

    Audio audio;
    volatile bool audio_playing;
    volatile bool audio_stopping;
    std::string next_stream_url;
    std::string bass_str;
    std::string mid_str;
    std::string tr_str;
};
