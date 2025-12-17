#include "aht21.h"
#include "i2c_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define AHT21_I2C_ADDRESS 0x38

#define AHT21_CMD_SOFT_RESET 0xBA
#define AHT21_CMD_INIT 0xBE
#define AHT21_CMD_TRIGGER 0xAC

static const char *TAG = "aht21";

static float temperature_c = 0.0f;
static float humidity_rh = 0.0f;
static uint8_t last_status = 0;

static esp_err_t aht21_write_command(uint8_t cmd, const uint8_t *data, size_t len)
{
    uint8_t buffer[3] = {cmd, 0x00, 0x00};
    if (data && len > 0) {
        for (size_t i = 0; i < len && (i + 1) < sizeof(buffer); i++) {
            buffer[i + 1] = data[i];
        }
    }
    return i2c_driver_write(AHT21_I2C_ADDRESS, buffer, len + 1);
}

void aht21_init(void)
{
    ESP_LOGI(TAG, "Initializing AHT21");

    // Soft reset to ensure a clean start
    aht21_write_command(AHT21_CMD_SOFT_RESET, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(20));

    // Initialize for normal measurement mode
    uint8_t init_data[2] = {0x08, 0x00};
    aht21_write_command(AHT21_CMD_INIT, init_data, sizeof(init_data));
    vTaskDelay(pdMS_TO_TICKS(20));
}

void aht21_read_data(void)
{
    uint8_t trigger_data[2] = {0x33, 0x00};
    aht21_write_command(AHT21_CMD_TRIGGER, trigger_data, sizeof(trigger_data));

    // Typical measurement time ~80ms
    vTaskDelay(pdMS_TO_TICKS(100));

    uint8_t raw_data[6] = {0};
    if (i2c_driver_read(AHT21_I2C_ADDRESS, NULL, 0, raw_data, sizeof(raw_data)) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read measurement");
        return;
    }

    last_status = raw_data[0];

    uint32_t raw_humidity = ((uint32_t)raw_data[1] << 12) |
                            ((uint32_t)raw_data[2] << 4) |
                            ((uint32_t)raw_data[3] >> 4);
    uint32_t raw_temperature = (((uint32_t)raw_data[3] & 0x0F) << 16) |
                               ((uint32_t)raw_data[4] << 8) |
                               (uint32_t)raw_data[5];

    humidity_rh = ((float)raw_humidity / 1048576.0f) * 100.0f;
    temperature_c = ((float)raw_temperature / 1048576.0f) * 200.0f - 50.0f;

    ESP_LOGI(TAG, "Status: 0x%02X, Temp: %.2fC, Humidity: %.2f%%", last_status, temperature_c, humidity_rh);
}

float aht21_get_temperature_c(void)
{
    return temperature_c;
}

float aht21_get_humidity(void)
{
    return humidity_rh;
}

uint8_t aht21_get_status(void)
{
    return last_status;
}

void aht21_get_ens16x_compensation(uint8_t *t, uint8_t *h)
{
    if (!t || !h) {
        return;
    }

    float temperature_k = temperature_c + 273.15f;
    uint16_t encoded_temp = (uint16_t)(temperature_k * 64.0f + 0.5f);
    uint16_t encoded_humidity = (uint16_t)(humidity_rh * 512.0f + 0.5f);

    t[0] = encoded_temp & 0xFF;
    t[1] = (encoded_temp >> 8) & 0xFF;
    h[0] = encoded_humidity & 0xFF;
    h[1] = (encoded_humidity >> 8) & 0xFF;
}
