#pragma once

/**
 * @file speech_util.h
 * @brief Provides utilities for local speech synthesis handling and configuration management.
 */

#include "audio_util.h"
#include <vector>
#include <string>
#include "cJSON.h"
#include "esp_log.h"
#include "fs_util.h"
#include "context.h"

/**
 * @class SpeechUtil
 * @brief Singleton class for managing speech synthesis configuration and task handling.
 *
 * This class handles communication with a local TTS (Text-to-Speech) server, manages synthesis task lifecycle,
 * and updates configuration from the filesystem or network.
 */
class SpeechUtil
{
private:
    /**
     * @brief Private constructor to enforce singleton pattern.
     */
    SpeechUtil();

    // Disable copy/assignment
    SpeechUtil(const SpeechUtil &) = delete;
    SpeechUtil &operator=(const SpeechUtil &) = delete;

    /**
     * @brief Identifies speech chunk breaks based on punctuation or length.
     * @param text Input text to parse.
     * @param len Desired chunk length.
     * @param chunks Vector of previous break points.
     * @return Vector of new chunk break indices.
     */
    std::vector<int> identifySpeechChunks(const std::string &text, int len, const std::vector<int> &chunks);

    /**
     * @brief Gets the endpoint index of a chunk from `chunks` array.
     * @param chunks Vector of chunk breaks.
     * @param i Index in the vector.
     * @return End position in the text.
     */
    int getEndPosition(const std::vector<int> &chunks, int i);

    Context *ctx ; ///< Global context instance

public:
    /**
     * @brief Accessor to singleton instance.
     * @return Pointer to `SpeechUtil` instance.
     */
    static SpeechUtil *getInstance();

    /**
     * @brief Initializes any state required for speech handling.
     */
    void setupSpeechUtil();

    /**
     * @brief Update TTS server hostname.
     * @param host New hostname or IP.
     * @return True on success.
     */
    bool updateSpeechHost(const std::string &host);

    /**
     * @brief Update TTS server port.
     * @param port New port number.
     * @return True on success.
     */
    bool updateSpeechPort(int port);

    /**
     * @brief Update TTS server path.
     * @param path New request path.
     * @return True on success.
     */
    bool updateSpeechPath(const std::string &path);

    /**
     * @brief Task function that handles speech synthesis from text.
     * @param param Parameters passed to the task (usually a struct).
     */
    static void speech_synthesis_task(void *param);

    /**
     * @brief Task function for long-form speech synthesis (e.g., paragraph/streamed input).
     * @param param Parameters passed to the task.
     */
    static void lspeech_synthesis_task(void *param);

    /**
     * @brief Parses and handles config updates from HTTP queries.
     * @param query Query string key.
     * @param value New value to set.
     * @return True on success.
     */
    bool handleSingleConfigUpdate(const std::string &query, const std::string &value);

    /// Default local TTS configuration values
    std::string &local_tts_host = ctx->local_tts_host;  ///< Default TTS server IP
    int &local_tts_port = ctx->local_tts_port;          ///< Default TTS server port
    std::string &local_tts_path = ctx->local_tts_path;  ///< Default TTS request path

    /// Setters for local TTS config
    void setLocalTTSHost(const std::string &host);
    void setLocalTTSPort(int port);
    void setLocalTTSTPath(const std::string &path);

    /// Getters for local TTS config
    std::string getLocalTTSHost() const;
    int getLocalTTSPort() const;
    std::string getLocalTTSTPath() const;

    /**
     * @brief Check if the main speech task is running.
     * @return True if running.
     */
    bool speech_task_running();

    /**
     * @brief Stop the main speech synthesis task.
     * @return True on success.
     */
    bool stop_speech_task();

    /**
     * @brief Update all TTS configuration settings at once.
     * @param host New hostname.
     * @param port New port.
     * @param path New request path.
     */
    void updateSpeechConfig(const std::string &host, int port, const std::string &path);

    /**
     * @brief Check if the long-form speech task is running.
     * @return True if running.
     */
    bool speech_ltask_running();

    /**
     * @brief Stop the long-form speech synthesis task.
     * @return True on success.
     */
    bool stop_lspeech_task();

    /**
     * @brief Stop all running speech-related tasks.
     * @return True if all were successfully stopped.
     */
    bool stopAllPreviousTasks();
};
