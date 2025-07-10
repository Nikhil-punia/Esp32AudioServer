#pragma once

#include "audio_util.h"
#include <vector>
#include <string>
#include "cJSON.h"
#include "esp_log.h"
#include "fs_util.h"
#include "structGlobal.h"


class SpeechUtil
{

private:
    SpeechUtil() {} // private constructor
    SpeechUtil(const SpeechUtil &) = delete;
    SpeechUtil &operator=(const SpeechUtil &) = delete;

    std::vector<int> identifySpeechChunks(const std::string &text, int len, const std::vector<int> &chunks);
    int getEndPosition(const std::vector<int> &chunks, int i);

public:
    static SpeechUtil *getInstance();

    void setupSpeechUtil();

    bool updateSpeechHost(const std::string &host);
    bool updateSpeechPort(int port);
    bool updateSpeechPath(const std::string &path);

    static void speech_synthesis_task(void *param);
    static void lspeech_synthesis_task(void *param);
    bool handleSingleConfigUpdate(const std::string &query, const std::string &value);

    std::string local_tts_host = "192.168.154.220";
    int local_tts_port = 5000;
    std::string local_tts_path = "/edge_tts";

    void setLocalTTSHost(const std::string &host) { local_tts_host = host; }
    void setLocalTTSPort(int port) { local_tts_port = port; }
    void setLocalTTSTPath(const std::string &path) { local_tts_path = path; }

    std::string getLocalTTSHost() const { return local_tts_host; }
    int getLocalTTSPort() const { return local_tts_port; }
    std::string getLocalTTSTPath() const { return local_tts_path; }

    bool speech_task_running();
    void updateSpeechConfig(const std::string &host, int port, const std::string &path);
    bool stop_speech_task();

    bool speech_ltask_running();
    bool stop_lspeech_task();
};

