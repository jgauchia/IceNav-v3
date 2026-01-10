/**
 * @file styles.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Shared styles definitions
 * @version 0.2.4
 * @date 2025-01
 */

#pragma once

#include <lvgl.h>

/**
 * @brief Shared LVGL styles for consistent UI appearance
 */
extern lv_style_t styleTransparent;     /**< Transparent container (bg_opa=0, border_opa=0) */
extern lv_style_t styleMapWidget;       /**< Semi-transparent map overlay widget */
extern lv_style_t styleFloatingBar;     /**< Floating button bar style */
extern lv_style_t styleScrollbarWhite;  /**< White scrollbar for tileview */

/**
 * @brief Initialize all shared styles
 * @note Must be called once during LVGL initialization, after lv_init()
 */
void initSharedStyles();
