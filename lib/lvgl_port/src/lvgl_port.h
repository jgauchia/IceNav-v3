/**
 * @file lvgl_port.h
 * @brief LVGL 9.x port for ESP-IDF with LovyanGFX
 */

#pragma once

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LVGL port (display + touch)
 * @return ESP_OK on success
 */
esp_err_t lvgl_port_init(void);

/**
 * @brief LVGL task handler - call periodically
 * @param max_ms maximum time to spend in ms
 * @return time until next call needed in ms
 */
uint32_t lvgl_port_task_handler(uint32_t max_ms);

/**
 * @brief Lock LVGL mutex (for thread safety)
 * @param timeout_ms timeout in ms (-1 for infinite)
 * @return true if locked, false on timeout
 */
bool lvgl_port_lock(int timeout_ms);

/**
 * @brief Unlock LVGL mutex
 */
void lvgl_port_unlock(void);

/**
 * @brief Get display handle
 * @return LVGL display pointer
 */
lv_display_t *lvgl_port_get_display(void);

/**
 * @brief Get touch input device handle
 * @return LVGL input device pointer
 */
lv_indev_t *lvgl_port_get_touch(void);

#ifdef __cplusplus
}
#endif
