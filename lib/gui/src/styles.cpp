/**
 * @file styles.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Shared styles implementation
 * @version 0.2.9
 * @date 2026-06
 */

#include "styles.hpp"

lv_style_t styleTransparent;     /**< Transparent container style */
lv_style_t styleMapWidget;       /**< Semi-transparent map overlay widget style */
lv_style_t styleScrollbarWhite;  /**< White scrollbar style */
lv_style_t styleScrim;           /**< Full screen dim scrim style */
lv_style_t styleBottomSheet;     /**< Bottom sheet panel style */

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

    // White scrollbar for tileview
    lv_style_init(&styleScrollbarWhite);
    lv_style_set_bg_color(&styleScrollbarWhite, lv_color_hex(0xFFFFFF));

    // Full screen dim scrim behind the bottom sheet
    lv_style_init(&styleScrim);
    lv_style_set_bg_color(&styleScrim, lv_color_black());
    lv_style_set_bg_opa(&styleScrim, LV_OPA_50);
    lv_style_set_border_width(&styleScrim, 0);
    lv_style_set_radius(&styleScrim, 0);
    lv_style_set_pad_all(&styleScrim, 0);

    // Bottom sheet panel (rounded top corners, dark background)
    lv_style_init(&styleBottomSheet);
    lv_style_set_bg_color(&styleBottomSheet, lv_color_black());
    lv_style_set_bg_opa(&styleBottomSheet, 235);
    lv_style_set_border_color(&styleBottomSheet, lv_color_white());
    lv_style_set_border_width(&styleBottomSheet, 1);
    lv_style_set_border_opa(&styleBottomSheet, LV_OPA_20);
    lv_style_set_border_side(&styleBottomSheet, LV_BORDER_SIDE_TOP);
    lv_style_set_radius(&styleBottomSheet, 18);
}
