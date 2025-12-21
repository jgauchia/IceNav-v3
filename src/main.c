/**
 * @file main.c
 * @brief IceNav ESP32 GPS Navigator - ESP-IDF Entry Point
 * @details Phase 5: Sensors integration (battery, compass, IMU, barometer)
 */

#include <stdio.h>
#include <string.h>
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
#include "sensors.h"

static const char *TAG = "icenav";

// LVGL labels for sensor display
static lv_obj_t *lbl_battery = NULL;
static lv_obj_t *lbl_compass = NULL;
static lv_obj_t *lbl_imu = NULL;
static lv_obj_t *lbl_bme280 = NULL;
static lv_obj_t *lbl_gps = NULL;

/**
 * @brief Create the main screen with sensor information
 */
static void create_main_screen(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a2e), 0);

    // Title label
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "IceNav GPS Navigator");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x00ff88), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Subtitle
    lv_obj_t *subtitle = lv_label_create(scr);
    lv_label_set_text(subtitle, "ESP-IDF Migration - Phase 5: Sensors");
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0xaaaaaa), 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 40);

    // Sensor info container
    lv_obj_t *container = lv_obj_create(scr);
    lv_obj_set_size(container, 300, 280);
    lv_obj_align(container, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x16213e), 0);
    lv_obj_set_style_border_color(container, lv_color_hex(0x0f3460), 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(container, 10, 0);
    lv_obj_set_style_pad_row(container, 8, 0);

    // Battery label
    lbl_battery = lv_label_create(container);
    lv_label_set_text(lbl_battery, "Battery: --");
    lv_obj_set_style_text_color(lbl_battery, lv_color_hex(0xffd700), 0);

    // Compass label
    lbl_compass = lv_label_create(container);
    lv_label_set_text(lbl_compass, "Compass: --");
    lv_obj_set_style_text_color(lbl_compass, lv_color_hex(0x00bfff), 0);

    // IMU label
    lbl_imu = lv_label_create(container);
    lv_label_set_text(lbl_imu, "IMU: --");
    lv_obj_set_style_text_color(lbl_imu, lv_color_hex(0xff6b6b), 0);

    // BME280 label
    lbl_bme280 = lv_label_create(container);
    lv_label_set_text(lbl_bme280, "BME280: --");
    lv_obj_set_style_text_color(lbl_bme280, lv_color_hex(0x4ecdc4), 0);

    // GPS label
    lbl_gps = lv_label_create(container);
    lv_label_set_text(lbl_gps, "GPS: UART initialized");
    lv_obj_set_style_text_color(lbl_gps, lv_color_hex(0x95e1d3), 0);

    // System info
    char buf[64];
    snprintf(buf, sizeof(buf), "Display: %dx%d", display_width(), display_height());
    lv_obj_t *info_disp = lv_label_create(container);
    lv_label_set_text(info_disp, buf);
    lv_obj_set_style_text_color(info_disp, lv_color_hex(0x888888), 0);

    snprintf(buf, sizeof(buf), "Free heap: %u KB", (unsigned)(esp_get_free_heap_size() / 1024));
    lv_obj_t *info_heap = lv_label_create(container);
    lv_label_set_text(info_heap, buf);
    lv_obj_set_style_text_color(info_heap, lv_color_hex(0x888888), 0);

    size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    snprintf(buf, sizeof(buf), "PSRAM free: %u KB", (unsigned)(psram_free / 1024));
    lv_obj_t *info_psram = lv_label_create(container);
    lv_label_set_text(info_psram, buf);
    lv_obj_set_style_text_color(info_psram, lv_color_hex(0x888888), 0);
}

/**
 * @brief Update sensor values on screen
 */
static void update_sensor_display(void)
{
    char buf[128];

    // Battery
    float voltage = battery_read_voltage();
    float percentage = battery_read_percentage();
    if (voltage > 0) {
        snprintf(buf, sizeof(buf), "Battery: %.2fV (%.0f%%)", voltage, percentage);
    } else {
        snprintf(buf, sizeof(buf), "Battery: N/A");
    }
    lv_label_set_text(lbl_battery, buf);

    // Compass
    if (compass_is_connected()) {
        int heading = compass_get_heading(0, 0, 0.22f);  // Default declination
        snprintf(buf, sizeof(buf), "Compass: %d deg", heading);
    } else {
        snprintf(buf, sizeof(buf), "Compass: Not connected");
    }
    lv_label_set_text(lbl_compass, buf);

    // IMU
    if (imu_is_connected()) {
        imu_accel_t accel;
        if (imu_read_accel(&accel) == ESP_OK) {
            snprintf(buf, sizeof(buf), "IMU: X=%.2f Y=%.2f Z=%.2f g", accel.x, accel.y, accel.z);
        } else {
            snprintf(buf, sizeof(buf), "IMU: Read error");
        }
    } else {
        snprintf(buf, sizeof(buf), "IMU: Not connected");
    }
    lv_label_set_text(lbl_imu, buf);

    // BME280
    if (bme280_is_connected()) {
        bme280_data_t data;
        if (bme280_read(&data) == ESP_OK) {
            snprintf(buf, sizeof(buf), "BME: %.1fC %.0fhPa %.0f%%",
                     data.temperature, data.pressure, data.humidity);
        } else {
            snprintf(buf, sizeof(buf), "BME280: Read error");
        }
    } else {
        snprintf(buf, sizeof(buf), "BME280: Not connected");
    }
    lv_label_set_text(lbl_bme280, buf);
}

/**
 * @brief Sensor monitoring task - prints status to console periodically
 */
static void sensor_monitor_task(void *arg)
{
    ESP_LOGI(TAG, "Sensor monitor task started");

    while (1) {
        // Print sensor status to console
        sensors_print_status();

        // Wait 5 seconds between readings
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * @brief LVGL task with sensor display updates
 */
static void lvgl_task(void *arg)
{
    ESP_LOGI(TAG, "LVGL task started");

    uint32_t sensor_update_counter = 0;

    while (1) {
        if (lvgl_port_lock(100)) {
            // Handle LVGL timer
            uint32_t time_till_next = lvgl_port_task_handler(10);

            // Update sensor display every 1 second (approx 100 iterations)
            if (++sensor_update_counter >= 100) {
                sensor_update_counter = 0;
                update_sensor_display();
            }

            lvgl_port_unlock();

            // Use LVGL's suggested delay, minimum 10ms
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
    ESP_LOGI(TAG, "ESP-IDF Migration - Phase 5");
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

    // Initialize board (I2C, SPI, UART for GPS)
    ESP_LOGI(TAG, "Initializing board...");
    esp_err_t ret = board_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Board initialization failed!");
        return;
    }

    // Initialize display
    ESP_LOGI(TAG, "Initializing display...");
    ret = display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Display initialization failed!");
        return;
    }

    // Initialize LVGL port
    ESP_LOGI(TAG, "Initializing LVGL...");
    ret = lvgl_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LVGL port initialization failed!");
        return;
    }

    // Initialize all sensors
    ESP_LOGI(TAG, "Initializing sensors...");
    sensors_init_all();

    // Create main screen
    if (lvgl_port_lock(-1)) {
        create_main_screen();
        lvgl_port_unlock();
    }

    // Create LVGL task (includes sensor display updates)
    xTaskCreatePinnedToCore(lvgl_task, "lvgl", 8192, NULL, 2, NULL, 1);

    // Create sensor monitor task (console output)
    xTaskCreatePinnedToCore(sensor_monitor_task, "sensors", 4096, NULL, 1, NULL, 0);

    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "System ready!");
    ESP_LOGI(TAG, "- Display: %dx%d", display_width(), display_height());
    ESP_LOGI(TAG, "- LVGL: Running");
    ESP_LOGI(TAG, "- GPS: UART%d @ %d baud", BOARD_GPS_UART_NUM, BOARD_GPS_BAUD);
    ESP_LOGI(TAG, "- Sensors: Monitoring");
    ESP_LOGI(TAG, "Free heap: %u KB", (unsigned)(esp_get_free_heap_size() / 1024));
    ESP_LOGI(TAG, "================================");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
