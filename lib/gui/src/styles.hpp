/**
 * @file styles.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Shared styles definitions
 * @version 0.2.5
 * @date 2025-01
 */

#pragma once

#include <lvgl.h>

extern lv_style_t styleTransparent;     /**< Transparent container (bg_opa=0, border_opa=0) */
extern lv_style_t styleMapWidget;       /**< Semi-transparent map overlay widget */
extern lv_style_t styleFloatingBar;     /**< Floating button bar style */
extern lv_style_t styleScrollbarWhite;  /**< White scrollbar for tileview */

void initSharedStyles();
