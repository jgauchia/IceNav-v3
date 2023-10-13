/**
 * @file lvgl_funcs.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL custom functions
 * @version 0.1.6
 * @date 2023-06-14
 */

/**
 * @brief Custom LVGL function to hide cursor
 *
 * @param LVGL object
 */
static void lv_obj_hide_cursor(_lv_obj_t *obj)
{
    static lv_style_t style1;
    lv_style_init(&style1);
    lv_style_set_bg_opa(&style1, LV_OPA_TRANSP);
    lv_style_set_text_opa(&style1, LV_OPA_TRANSP);
    lv_obj_add_style(obj, &style1, LV_PART_CURSOR);

    static lv_style_t style2;
    lv_style_init(&style2);
    lv_style_set_bg_opa(&style2, LV_OPA_100);
    lv_style_set_text_opa(&style2, LV_OPA_100);
    lv_obj_add_style(obj, &style2, LV_PART_CURSOR | LV_STATE_FOCUS_KEY);
    lv_obj_add_style(obj, &style2, LV_PART_CURSOR | LV_STATE_FOCUSED);
}

/**
 * @brief Custom LVGL function to select widget
 *
 * @param obj
 */
static void select_obj(_lv_obj_t *obj)
{
    static lv_style_t style_widget;
    lv_style_init(&style_widget);
    lv_style_set_bg_opa(&style_widget, LV_OPA_80);
    lv_style_set_border_opa(&style_widget, LV_OPA_100);
    lv_obj_add_style(obj, &style_widget, LV_PART_MAIN);
}

/**
 * @brief Custom LVGL function to unselect widget
 * 
 * @param obj 
 */
static void unselect_obj(_lv_obj_t *obj)
{
    static lv_style_t style_widget;
    lv_style_init(&style_widget);
    lv_style_set_bg_opa(&style_widget, LV_OPA_0);
    lv_style_set_border_opa(&style_widget, LV_OPA_0);
    lv_obj_add_style(obj, &style_widget, LV_PART_MAIN);
}