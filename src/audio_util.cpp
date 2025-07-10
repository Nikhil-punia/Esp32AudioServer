#include "audio_util.h"

// Constructor implementation
AudioUtil::AudioUtil()
    : audio(),
      audio_playing(false),
      audio_stopping(false)
{
    next_stream_url[0] = '\0';
    bass_str[0] = '\0';
    mid_str[0] = '\0';
    tr_str[0] = '\0';
}

void AudioUtil::setupAudio()
{
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setConnectionTimeout(30000, 30000); // Set connection timeout for HTTP and HTTPS
    audio.setVolume(21); // Set default volume (range 0-21)
}

void AudioUtil::handle_music_request(const char *url)
{
    if (audio.isRunning() || audio_playing)
    {
        Serial.println("Stopping current song to switch streams...");
        audio_playing = false;
        audio_stopping = true;
        audio.stopSong();
        next_stream_url = url;
    }
    else
    {
        Serial.printf("Connecting to new host: %s\n", url);
        if (audio.connecttohost(url))
        {
            audio_playing = true;
            Serial.println("Successfully connected to host.");
        }
        else
        {
            audio_playing = false;
            Serial.println("Failed to connect to host.");
        }
        next_stream_url[0] = '\0';
    }
}

void AudioUtil::handle_google_tts(const char *text, const char *lang)
{
    if (audio.isRunning() || audio_playing)
    {
        Serial.println("Stopping current song to switch streams...");
        audio_playing = false;
        audio_stopping = true;
        audio.stopSong();
    }

    Serial.print("Connecting to Google TTS ");
    if (audio.connecttospeech(text, lang))
    {
        audio_playing = true;
        Serial.println("Successfully connected to Google TTS.");
    }
    else
    {
        audio_playing = false;
        Serial.println("Failed to connect to Google TTS.");
    }
}

void AudioUtil::handle_local_tts(std::string text, std::string voice_id)
{
    // Access local_tts_host from speech_util.h

    if (audio.isRunning() || audio_playing)
    {
        Serial.println("Stopping current song to switch streams...");
        audio_playing = false;
        audio_stopping = true;
        audio.stopSong();
    }

    Serial.printf("Connecting to Local TTS with text: %s and voice_id: %s\n", text.c_str(), voice_id.c_str());
    Serial.print("Connecting to Local TTS ");
    
    int slashIndex = speechUtil.local_tts_path.indexOf('/');
    String endpoint = (slashIndex >= 0) ? speechUtil.local_tts_path.substring(slashIndex + 1) : "";
    if (audio.connect_local_tts(speechUtil.local_tts_host, speechUtil.local_tts_port, speechUtil.local_tts_path, String(text.c_str()), String(voice_id.c_str()), "", endpoint))
    {
        audio_playing = true;
        Serial.println("Successfully connected to Local TTS.");
    }
    else
    {
        audio_playing = false;
        Serial.println("Failed to connect to Local TTS.");
    }
}



void AudioUtil::setTone(int b, int m, int t)
{
    audio.setTone(b, m, t);
    Serial.printf("Setting tone: Bass=%d, Mid=%d, Treble=%d\n", b, m, t);
}

void AudioUtil::loopAudio()
{
    if (audio_stopping)
    {
        if (!audio.isRunning() && audio.getCodec() == 0)
        {
            Serial.println("Audio stopped and ready for new connection.");
            audio_stopping = false;
            if (next_stream_url.length() > 0)
            {
                Serial.printf("Connecting to pending stream: %s\n", next_stream_url.c_str());
                if (audio.connecttohost(next_stream_url.c_str()))
                {
                    audio_playing = true;
                    Serial.println("Successfully connected to pending host.");
                }
                else
                {
                    audio_playing = false;
                    Serial.println("Failed to connect to pending host.");
                }
                next_stream_url.clear();
            }
        }
        else
        {
            vTaskDelay(1);
            return;
        }
    }

    if (audio_playing)
    {
        audio.loop();
        vTaskDelay(1);
    }
}

void AudioUtil::stopAudio()
{
    if (audio.isRunning())
    {
        audio.stopSong();
        audio_playing = false;
        audio_stopping = false;
        Serial.println("Audio stopped.");
    }
    else
    {
        Serial.println("No audio is currently playing.");
    }
}

// bool Audio::connect_local_tts(const String& host, int port, const String& path,
//                              const String& text, const String& voice_id,
//                               const String& lang, const String& endpoint) {

//     if (text.isEmpty()) {
//         AUDIO_INFO("TTS text is empty");
//         stopSong();
//         return false;
//     }

//     xSemaphoreTakeRecursive(mutex_playAudioData, 0.3 * configTICK_RATE_HZ);
//     setDefaults();
//     m_f_ssl = false;
//     m_f_tts = true;
//     m_speechtxt.assign(text.c_str());

//     String body = "{";
//     body += "\"text\":\"" + text + "\"";
//     if (!voice_id.isEmpty()) body += ",\"voice\":\"" + voice_id + "\"";
//     if (!lang.isEmpty()) body += ",\"lang\":\"" + lang + "\"";
//     body += "}";

//     String req = "POST " + path + " HTTP/1.1\r\n";
//     req += "Host: " + host + "\r\n";
//     req += "User-Agent: ESP32-AudioI2S\r\n";
//     req += "Accept: */*\r\n";
//     req += "Content-Type: application/json\r\n";
//     req += "Content-Length: " + String(body.length()) + "\r\n";
//     req += "Connection: close\r\n\r\n";
//     req += body;

//     _client = static_cast<WiFiClient*>(&client);
     
//     AUDIO_INFO("Connecting to local TTS server %s:%d", host.c_str(), port);

//     if (!_client->connect(host.c_str(), port)) {
//         AUDIO_ERROR("Connection failed");
//         xSemaphoreGiveRecursive(mutex_playAudioData);
//         return false;
//     }

    
//     _client->print(req);

//     m_f_running = true;
//     m_dataMode = HTTP_RESPONSE_HEADER;
//     m_lastHost.assign(host.c_str());
//     m_currentHost.assign(host.c_str());

//     if (endpoint == "gtts" || endpoint == "edge_tts") {
//         m_expectedCodec = CODEC_MP3;
//     } else if (endpoint == "tts") {
//         m_expectedCodec = CODEC_WAV;
//     }

//     xSemaphoreGiveRecursive(mutex_playAudioData);
//     return true;
// }
