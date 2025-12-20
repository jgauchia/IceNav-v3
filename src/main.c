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
#include "esp_heap_caps.h"
#include "esp_psram.h"
#include "board.h"
#include "display.h"
#include "lvgl_port.h"
#include "lvgl.h"

static const char *TAG = "icenav";

/**
 * @brief Create a simple LVGL demo screen
 */
static void create_demo_screen(void)
{
    lv_obj_t *scr = lv_screen_active();

    // Title label
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "IceNav GPS Navigator");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Subtitle
    lv_obj_t *subtitle = lv_label_create(scr);
    lv_label_set_text(subtitle, "ESP-IDF + LVGL Migration");
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_16, 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 60);

    // Info box
    lv_obj_t *info_box = lv_obj_create(scr);
    lv_obj_set_size(info_box, 280, 150);
    lv_obj_align(info_box, LV_ALIGN_CENTER, 0, 20);

    char buf[64];
    snprintf(buf, sizeof(buf), "Display: %dx%d", display_width(), display_height());
    lv_obj_t *info1 = lv_label_create(info_box);
    lv_label_set_text(info1, buf);
    lv_obj_align(info1, LV_ALIGN_TOP_LEFT, 10, 10);

    snprintf(buf, sizeof(buf), "Free heap: %u KB", (unsigned)(esp_get_free_heap_size() / 1024));
    lv_obj_t *info2 = lv_label_create(info_box);
    lv_label_set_text(info2, buf);
    lv_obj_align(info2, LV_ALIGN_TOP_LEFT, 10, 40);

    lv_obj_t *info3 = lv_label_create(info_box);
    lv_label_set_text(info3, "Phase 3: LVGL OK");
    lv_obj_set_style_text_color(info3, lv_color_hex(0x00FF00), 0);
    lv_obj_align(info3, LV_ALIGN_TOP_LEFT, 10, 70);

    // Touch test button
    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 200, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -30);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Touch Test");
    lv_obj_center(btn_label);
}

/**
 * @brief LVGL task
 */
static void lvgl_task(void *arg)
{
    ESP_LOGI(TAG, "LVGL task started");

    while (1) {
        if (lvgl_port_lock(100)) {
            uint32_t time_till_next = lvgl_port_task_handler(10);
            lvgl_port_unlock();
            // Use LVGL's suggested delay, minimum 10ms to feed watchdog
            uint32_t delay_ms = (time_till_next < 10) ? 10 : time_till_next;
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

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
    ESP_LOGI(TAG, "Free heap: %u KB", (unsigned)(esp_get_free_heap_size() / 1024));

    // PSRAM info
#if CONFIG_SPIRAM
    size_t psram_size = esp_psram_get_size();
    ESP_LOGI(TAG, "PSRAM: %u MB total", (unsigned)(psram_size / (1024 * 1024)));
    ESP_LOGI(TAG, "PSRAM free: %u KB", (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
#else
    ESP_LOGW(TAG, "PSRAM: Not enabled in config");
    size_t spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    if (spiram_free > 0) {
        ESP_LOGI(TAG, "PSRAM free: %u KB", (unsigned)(spiram_free / 1024));
    }
#endif

    // Initialize board (I2C, SPI, UART)
    esp_err_t ret = board_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Board initialization failed!");
        return;
    }

    // Initialize display
    ret = display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Display initialization failed!");
        return;
    }

    // Initialize LVGL port
    ret = lvgl_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LVGL port initialization failed!");
        return;
    }

    // Create demo screen
    if (lvgl_port_lock(-1)) {
        create_demo_screen();
        lvgl_port_unlock();
    }

    // Create LVGL task
    xTaskCreatePinnedToCore(lvgl_task, "lvgl", 8192, NULL, 2, NULL, 1);

    ESP_LOGI(TAG, "System ready");
    ESP_LOGI(TAG, "Free heap after init: %u KB", (unsigned)(esp_get_free_heap_size() / 1024));

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
