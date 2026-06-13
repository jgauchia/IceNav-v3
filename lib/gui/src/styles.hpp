/**
 * @file styles.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Shared styles definitions
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include <lvgl.h>

extern lv_style_t styleTransparent;     /**< Transparent container (bg_opa=0, border_opa=0) */
extern lv_style_t styleMapWidget;       /**< Semi-transparent map overlay widget */
extern lv_style_t styleScrollbarWhite;  /**< White scrollbar for tileview */
extern lv_style_t styleScrim;           /**< Full screen dim scrim behind bottom sheet */
extern lv_style_t styleBottomSheet;     /**< Bottom sheet panel style */

void initSharedStyles();
