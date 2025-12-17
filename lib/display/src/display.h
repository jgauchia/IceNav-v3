/**
 * @file display.h
 * @brief Display driver using LovyanGFX
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize display
 * @return ESP_OK on success
 */
esp_err_t display_init(void);

/**
 * @brief Set backlight brightness
 * @param brightness 0-255
 */
void display_set_backlight(uint8_t brightness);

/**
 * @brief Get display width
 * @return width in pixels
 */
int display_width(void);

/**
 * @brief Get display height
 * @return height in pixels
 */
int display_height(void);

/**
 * @brief Fill screen with color (RGB565)
 * @param color RGB565 color
 */
void display_fill(uint16_t color);

/**
 * @brief Draw text on display
 * @param x X position
 * @param y Y position
 * @param text Text string
 */
void display_text(int x, int y, const char *text);

#ifdef __cplusplus
}
#endif
