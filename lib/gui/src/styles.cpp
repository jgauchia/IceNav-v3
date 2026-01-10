/**
 * @file styles.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Shared styles implementation
 * @version 0.2.4
 * @date 2025-01
 */

#include "styles.hpp"

lv_style_t styleTransparent;     /**< Transparent container style */
lv_style_t styleMapWidget;       /**< Semi-transparent map overlay widget style */
lv_style_t styleFloatingBar;     /**< Floating button bar style */
lv_style_t styleScrollbarWhite;  /**< White scrollbar style */

/**
 * @brief Initialize all shared styles
 *
 * @details Creates reusable styles to avoid redundant style definitions
 *          across multiple screen files. Call once after lv_init().
 */
void initSharedStyles()
{
    // Transparent container (settings, grids, notify bar)
    lv_style_init(&styleTransparent);
    lv_style_set_bg_opa(&styleTransparent, LV_OPA_0);
    lv_style_set_border_opa(&styleTransparent, LV_OPA_0);

    // Semi-transparent map widget (zoom, speed, compass, scale, turn-by-turn)
    lv_style_init(&styleMapWidget);
    lv_style_set_bg_color(&styleMapWidget, lv_color_black());
    lv_style_set_bg_opa(&styleMapWidget, 128);
    lv_style_set_border_color(&styleMapWidget, lv_color_black());
    lv_style_set_border_width(&styleMapWidget, 1);
    lv_style_set_border_opa(&styleMapWidget, 128);

    // Floating button bar (menu bar, options bar)
    lv_style_init(&styleFloatingBar);
    lv_style_set_radius(&styleFloatingBar, LV_RADIUS_CIRCLE);
    lv_style_set_border_color(&styleFloatingBar, lv_color_white());
    lv_style_set_border_width(&styleFloatingBar, 1);
    lv_style_set_border_opa(&styleFloatingBar, LV_OPA_20);
    lv_style_set_bg_color(&styleFloatingBar, lv_color_black());
    lv_style_set_bg_opa(&styleFloatingBar, 210);

    // White scrollbar for tileview
    lv_style_init(&styleScrollbarWhite);
    lv_style_set_bg_color(&styleScrollbarWhite, lv_color_hex(0xFFFFFF));
}
