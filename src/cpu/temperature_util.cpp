#include "cpu/temperature_util.h"
#include "driver/temperature_sensor.h"

static temperature_sensor_handle_t temp_sensor = NULL;
static bool temp_sensor_enabled = false;

float getTemperature() {
    if (temp_sensor == NULL) {
        temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 100);
        esp_err_t err = temperature_sensor_install(&temp_sensor_config, &temp_sensor);
        if (err != ESP_OK) {
            return -1;
        }
    }

    if (!temp_sensor_enabled) {
        if (temperature_sensor_enable(temp_sensor) == ESP_OK) {
            temp_sensor_enabled = true;
        } else {
            return -1;
        }
    }

    float tsens_out;
    if (temperature_sensor_get_celsius(temp_sensor, &tsens_out) == ESP_OK) {
        return tsens_out;
    }

    return -1; // Return -1 if unable to read temperature
}
