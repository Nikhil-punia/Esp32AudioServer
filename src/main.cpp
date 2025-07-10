#include "esp32AudioServer.h"

Esp32AudioServer AudioServer;


/**
 * @brief Arduino setup function. Runs once at device startup.
 */
void setup()
{
    Serial.begin(115200); // Initialize serial communication for debugging output
    Serial.print("A\n\n");
    Serial.println("----------------------------------");
    Serial.printf("ESP32 Chip: %s\n", ESP.getChipModel());
    Serial.printf("Arduino Version: %d.%d.%d\n", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
    Serial.printf("ESP-IDF Version: %d.%d.%d\n", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
    Serial.printf("ARDUINO_LOOP_STACK_SIZE %d words (32 bit)\n", CONFIG_ARDUINO_LOOP_STACK_SIZE);
    Serial.println("----------------------------------");
    Serial.print("\n\n");

    AudioServer.setupServer();
}

/**
 * @brief Arduino loop function. Runs continuously in a dedicated FreeRTOS task.
 */
void loop()
{
    AudioServer.loopServer();
    delay(1);
}

// Optional callback for general audio information messages
void audio_info(const char *info)
{
    Serial.print("info        ");
    Serial.println(info);
}

// Optional callback for audio warning messages (highly recommended for debugging)
void audio_warn(const char *warn)
{
    Serial.print("warn        ");
    Serial.println(warn);
}

// Optional callback for audio error messages (highly recommended for debugging)
void audio_error(const char *error)
{
    Serial.print("error       ");
    Serial.println(error);
}
