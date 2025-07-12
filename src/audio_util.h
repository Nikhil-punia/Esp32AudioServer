#pragma once

#include "Audio.h"
#include <cstring>     // For legacy string ops
#include <string>      // For std::string
#include "esp_log.h"
#include "cJSON.h"
#include "fs_util.h"
#include "context.h"


/**
 * @brief Singleton class to manage audio streaming and TTS playback.
 * 
 * Provides utility functions to initialize audio, handle music and TTS playback,
 * manage tone settings, and maintain audio state across the application.
 */
class AudioUtil
{
public:
    /**
     * @brief Get the singleton instance of AudioUtil.
     * 
     * @return AudioUtil* Pointer to the single shared instance.
     */
    static AudioUtil* getInstance();

    /**
     * @brief Stop the currently playing audio if running.
     */
    void stopAudio();

    /**
     * @brief Set the bass tone level as a string (e.g., "5").
     * 
     * @param bass Bass level string.
     */
    void setBassStr(const std::string &bass) { bass_str = bass; };

    /**
     * @brief Set the mid tone level as a string.
     * 
     * @param mid Mid level string.
     */
    void setMidStr(const std::string &mid) { mid_str = mid; };

    /**
     * @brief Set the treble tone level as a string.
     * 
     * @param tr Treble level string.
     */
    void setTrStr(const std::string &tr) { tr_str = tr; };

    /**
     * @brief Get reference to the underlying Audio object.
     * 
     * @return Audio& Reference to ESP32-AudioI2S instance.
     */
    Audio& getAudio() { return audio; };

    /// @return Current bass string value.
    const std::string& getBassStr() const { return bass_str; };

    /// @return Current mid string value.
    const std::string& getMidStr() const { return mid_str; };

    /// @return Current treble string value.
    const std::string& getTrStr() const { return tr_str; };

    /**
     * @brief Set the stream URL to be played next.
     * 
     * @param url HTTP/HTTPS stream URL.
     */
    void setStreamUrl(const std::string &url) { next_stream_url = url; };

    /// @return Current pending stream URL.
    const std::string& getStreamUrl() const { return next_stream_url; };

    /**
     * @brief Configure I2S pins and set default volume and timeouts.
     */
    void setupAudio();

    /**
     * @brief Start or queue an HTTP music stream.
     * 
     * @param url Stream URL to play.
     */
    void handle_music_request(const char *url);

    /**
     * @brief Play TTS from Google's public TTS API.
     * 
     * @param text Text to be converted to speech.
     * @param lang Language code (e.g., "en", "hi").
     */
    void handle_google_tts(const char *text, const char *lang);

    /**
     * @brief Play TTS audio from a custom local TTS server.
     * 
     * @param text Text to synthesize.
     * @param voice_id Optional voice ID.
     * @param host Hostname or IP of TTS server.
     * @param port Server port (e.g., 5000).
     * @param path API path (e.g., "/api/tts").
     */
    void handle_local_tts(std::string text, std::string voice_id, std::string host, int port, std::string path);

    /**
     * @brief Apply tone settings (EQ) directly.
     * 
     * @param b Bass value
     * @param m Mid value
     * @param t Treble value
     */
    void setTone(int b, int m, int t);

    /**
     * @brief Handle background audio stream logic.
     * 
     * Call this periodically in the main loop or audio task.
     */
    void loopAudio();

private:
    /// Private constructor to enforce singleton
    AudioUtil();


    /// Prevent copy construction
    AudioUtil(const AudioUtil &) = delete;

    /// Prevent assignment
    AudioUtil& operator=(const AudioUtil &) = delete;

    Context *ctx;      ///< Shared configuration context

    Audio audio;                    ///< Main audio engine instance (ESP32-AudioI2S)
    volatile bool audio_playing;   ///< True if audio is currently playing
    volatile bool audio_stopping;  ///< True if in the process of stopping

    std::string next_stream_url;   ///< Pending stream URL if stopping is in progress

    std::string bass_str;          ///< Bass EQ level as string
    std::string mid_str;           ///< Mid EQ level as string
    std::string tr_str;            ///< Treble EQ level as string
};
