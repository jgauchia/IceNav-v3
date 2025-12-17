/**
 * @file main.c
 * @brief IceNav ESP32 GPS Navigator - ESP-IDF Entry Point
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "board.h"

static const char *TAG = "icenav";

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "IceNav GPS Navigator");
    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "ESP-IDF: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "Chip: %s, Cores: %d", CONFIG_IDF_TARGET, chip_info.cores);
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());

    // Initialize board (I2C, SPI, UART)
    esp_err_t ret = board_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Board initialization failed!");
        return;
    }

    ESP_LOGI(TAG, "System ready");
    ESP_LOGI(TAG, "Free heap after init: %lu bytes", esp_get_free_heap_size());

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
