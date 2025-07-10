#pragma once

class AudioUtil; 

#include "audio_util.h"
#include <vector>
#include <string>
#include "cJSON.h"
#include "esp_log.h"
#include "fs_util.h"

class SpeechUtil
{

private:
    std::vector<int> identifySpeechChunks(const std::string &text, int len, const std::vector<int> &chunks);
    int getEndPosition(const std::vector<int> &chunks, int i);

public:
    void setupSpeechUtil();

    bool updateSpeechHost(const std::string &host);
    bool updateSpeechPort(int port);
    bool updateSpeechPath(const std::string &path);


    static void speech_synthesis_task(void *param);
    static void lspeech_synthesis_task(void *param);
    bool handleSingleConfigUpdate(const std::string &query, const std::string &value);

    struct lSynthesizeArgs
    {
        SpeechUtil *self;
        AudioUtil *audioUtil;
        std::unique_ptr<std::string> text;
        std::unique_ptr<std::string> voice_id;
    };

     struct SynthesizeArgs
    {
        SpeechUtil *self;
        AudioUtil *audioUtil;
        std::string *text;
        std::string *lang;
    };


    String local_tts_host = "192.168.154.220";
    int local_tts_port = 5000;
    String local_tts_path = "/edge_tts";

    
    void setLocalTTSHost(const String &host) { local_tts_host = host; };
    void setLocalTTSPort(int port) { local_tts_port = port; };
    void setLocalTTSTPath(const String &path) { local_tts_path = path; };
    String getLocalTTSHost() const { return local_tts_host; };
    int getLocalTTSPort() const { return local_tts_port; };
    String getLocalTTSTPath() const { return local_tts_path; };

    bool speech_task_running();
    void updateSpeechConfig(const std::string &host, int port, const std::string &path);
    bool stop_speech_task();

    bool speech_ltask_running();
    bool stop_lspeech_task();
    

};

extern SpeechUtil speechUtil;