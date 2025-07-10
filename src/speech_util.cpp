#include "speech_util.h"


SpeechUtil* SpeechUtil::getInstance() {
    static SpeechUtil instance;
    return &instance;
}


void SpeechUtil::speech_synthesis_task(void *param)
{
    

    SynthesizeArgs *args = static_cast<SynthesizeArgs *>(param); // Use static_cast for C++ style casting

    if (!args || !args->text || !args->lang)
    {
        // Only attempt to delete/free if args is not null
        if (args) {
            delete args->text; // Deleting a nullptr is safe
            delete args->lang; // Deleting a nullptr is safe
            free(args);
        }
        vTaskDelete(NULL); // Delete the current task
    }

    const std::string *text = args->text;
    const std::string *lang = args->lang;
    int len = text->length();
    int start = 0;
    const int CHUNK_SIZE = 150;
    std::vector<int> initial_chunks;

    int div = len / CHUNK_SIZE;
    int repeat_time = ((len - (div * CHUNK_SIZE)) > 0) ? div + 1 : div;

    for (int i = 0; i < repeat_time; i++)
    {
        int chunk_len = (len - start > CHUNK_SIZE) ? CHUNK_SIZE : (len - start);
        initial_chunks.push_back(chunk_len);
        start += chunk_len;
    }

    std::vector<int> speech_chunks = getInstance()->identifySpeechChunks(*text, len, initial_chunks);

    for (int i = 0; i < speech_chunks.size(); ++i)
    {
        int start_pos = (i == 0) ? 0 : getInstance()->getEndPosition(speech_chunks, i - 1);
        std::string chunk = text->substr(start_pos, speech_chunks[i]);
        AudioUtil::getInstance()->handle_google_tts(chunk.c_str(), lang->c_str());
        while (AudioUtil::getInstance()->getAudio().isRunning())
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    delete args->text;
    delete args->lang;
    free(args);
    vTaskDelete(NULL);
}

void SpeechUtil::lspeech_synthesis_task(void *param)
{
    lSynthesizeArgs *args = static_cast<lSynthesizeArgs *>(param); // Use static_cast for C++ style casting
    if (!args || !args->text || !args->voice_id)
    {
        // Only attempt to delete/free if args is not null
        if (args) {
            delete args;
        }
        vTaskDelete(NULL); // Delete the current task
    }

    const std::string *text = args->text.get();
    const std::string *voice_id = args->voice_id.get();
    int len = text->length();
    int start = 0;
    const int CHUNK_SIZE = 250;
    std::vector<int> initial_chunks;

    int div = len / CHUNK_SIZE;
    int repeat_time = ((len - (div * CHUNK_SIZE)) > 0) ? div + 1 : div;

    for (int i = 0; i < repeat_time; i++)
    {
        int chunk_len = (len - start > CHUNK_SIZE) ? CHUNK_SIZE : (len - start);
        initial_chunks.push_back(chunk_len);
        start += chunk_len;
    }

    std::vector<int> speech_chunks = getInstance()->identifySpeechChunks(*text, len, initial_chunks);

    for (int i = 0; i < speech_chunks.size(); ++i)
    {
        int start_pos = (i == 0) ? 0 : getInstance()->getEndPosition(speech_chunks, i - 1);
        std::string chunk = text->substr(start_pos, speech_chunks[i]);
        AudioUtil::getInstance()->handle_local_tts(chunk, *voice_id,getInstance()->local_tts_host,getInstance()->local_tts_port,getInstance()->local_tts_path);
        while (AudioUtil::getInstance()->getAudio().isRunning())
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    delete args;
    vTaskDelete(NULL);
}

std::vector<int> SpeechUtil::identifySpeechChunks(const std::string &text, int len, const std::vector<int> &chunks)
{
    std::vector<int> end_positions;
    std::vector<int> new_chunks;

    for (int i = 0; i < chunks.size(); ++i)
    {
        int end = getEndPosition(chunks, i);
        int start = end - chunks[i];
        int char_pos = end;

        for (int j = start; j < end && j < text.length(); ++j)
        {
            if (!std::isalnum(text[j]))
            {
                char_pos = j;
            }
        }
        end_positions.push_back(char_pos);
    }

    for (size_t i = 0; i < end_positions.size(); ++i)
    {
        if (i == end_positions.size() - 1)
            new_chunks.push_back(len - (i > 0 ? end_positions[i - 1] : 0));
        else
            new_chunks.push_back(end_positions[i] - (i > 0 ? end_positions[i - 1] : 0));
    }

    return new_chunks;
}

int SpeechUtil::getEndPosition(const std::vector<int> &chunks, int i)
{
    int end = 0;
    for (int j = 0; j <= i; ++j)
    {
        end += chunks[j];
    }
    return end;
}



void SpeechUtil::setupSpeechUtil()
{
    // Revive the settings from json file if any
    std::string host = fsUtil.getStringConfigValue(fsUtil.root_file_path, "speech", "local_tts_host");
    if (!host.empty())
    {
        local_tts_host = host; // assign as const char* if local_tts_host is const char*
    }
    else
    {
        Serial.println("No local_tts_host found in config.json, using default.");
        fsUtil.setStringConfigValue(fsUtil.root_file_path, "speech", "local_tts_host", local_tts_host);
    }

    int port = fsUtil.getIntConfigValue(fsUtil.root_file_path, "speech", "local_tts_port");
    if (port > 0)
    {
        local_tts_port = port;
    }
    else
    {
        Serial.println("No valid local_tts_port found in config.json, using default.");
        fsUtil.setIntConfigValue(fsUtil.root_file_path, "speech", "local_tts_port", local_tts_port);
    }

    std::string path = fsUtil.getStringConfigValue(fsUtil.root_file_path, "speech", "local_tts_path");
    if (!path.empty())
    {
        local_tts_path = path;
    }
    else
    {
        Serial.println("No local_tts_path found in config.json, using default.");
        fsUtil.setStringConfigValue(fsUtil.root_file_path, "speech", "local_tts_path", local_tts_path.c_str());
    }

    Serial.println("SpeechUtil setup complete.");
}



bool SpeechUtil::speech_task_running()
{
    TaskHandle_t task_handle = xTaskGetHandle("speech_synth");
    return (task_handle != NULL);
}

bool SpeechUtil::stop_speech_task()
{
    TaskHandle_t task_handle = xTaskGetHandle("speech_synth");
    if (task_handle != NULL)
    {
        vTaskDelete(task_handle);
        return true;
    }
    return false;
}

bool SpeechUtil::speech_ltask_running()
{
    TaskHandle_t task_handle = xTaskGetHandle("lspeech_synth");
    return (task_handle != NULL);
}

bool SpeechUtil::stop_lspeech_task()
{
    TaskHandle_t task_handle = xTaskGetHandle("lspeech_synth");
    
    if (task_handle != NULL)
    {
        vTaskDelete(task_handle);
        return true;
    }
    
    return false;
}

bool SpeechUtil::updateSpeechHost(const std::string& host)
{
    local_tts_host = host;
    bool result = fsUtil.setStringConfigValue(fsUtil.root_file_path, "speech", "local_tts_host", local_tts_host.c_str());
    Serial.println("SpeechUtil Host updated.");
    return result;
}

bool SpeechUtil::updateSpeechPort(int port)
{
    local_tts_port = port;
    bool result = fsUtil.setIntConfigValue(fsUtil.root_file_path, "speech", "local_tts_port", local_tts_port);
    Serial.println("SpeechUtil Port updated.");
    return result;
}

bool SpeechUtil::updateSpeechPath(const std::string& path)
{
    local_tts_path = path;
    bool result = fsUtil.setStringConfigValue(fsUtil.root_file_path, "speech", "local_tts_path", local_tts_path.c_str());
    Serial.println("SpeechUtil Path updated.");
    return result;
}

bool SpeechUtil::handleSingleConfigUpdate(const std::string &query, const std::string &value)
{
    if (query == "local_tts_host")
    {
        return updateSpeechHost(value);
    }
    else if (query == "local_tts_port")
    {
        int port = std::stoi(value);
        return updateSpeechPort(port);
    }
    else if (query == "local_tts_path")
    {
        return updateSpeechPath(value);
    }
    else
    {
        Serial.printf("Unknown query: %s\n", query.c_str());
        return false;
    }
}