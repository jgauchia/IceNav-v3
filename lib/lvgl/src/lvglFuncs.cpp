/**
 * @file lvglFuncs.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL custom functions
 * @version 0.2.3
 * @date 2025-11
 */

#include "lvglFuncs.hpp"

lv_obj_t *msgDialog;     /**< Message dialog object. */

/**
 * @brief Custom LVGL function to hide the object's cursor.
 *
 * @param obj Pointer to the LVGL object whose cursor will be hidden.
 */
void objHideCursor(_lv_obj_t *obj)
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
 * @brief Custom LVGL function to select a widget.
 *
 * @details Applies a highlight style to the specified LVGL object to indicate that it is selected.
 *
 * @param obj Pointer to the LVGL object to be selected.
 */
void objSelect(_lv_obj_t *obj)
{
	static lv_style_t styleWidget;
	lv_style_init(&styleWidget);
	lv_style_set_bg_color(&styleWidget, lv_color_hex(0xB8B8B8));
	lv_style_set_bg_opa(&styleWidget, LV_OPA_20);
	lv_style_set_border_opa(&styleWidget, LV_OPA_100);
	lv_obj_add_style(obj, &styleWidget, LV_PART_MAIN);
}

/**
 * @brief Custom LVGL function to unselect a widget.
 *
 * @details Removes the selection highlight by applying a transparent style to the specified LVGL object.
 *
 * @param obj Pointer to the LVGL object to be unselected.
 */
void objUnselect(_lv_obj_t *obj)
{
	static lv_style_t styleWidget;
	lv_style_init(&styleWidget);
	lv_style_set_bg_color(&styleWidget, lv_color_black());
	lv_style_set_bg_opa(&styleWidget, LV_OPA_0);
	lv_style_set_border_opa(&styleWidget, LV_OPA_0);
	lv_obj_add_style(obj, &styleWidget, LV_PART_MAIN);
}

/**
 * @brief Restart timer callback.
 *
 * @param timer Pointer to the LVGL timer object that triggered the callback.
 */
void restartTimerCb(lv_timer_t *timer)
{
	if (lv_timer_get_idle() != 0)
		ESP.restart();
}

/**
 * @brief Show restart screen.
 *
 */
void showRestartScr()
{
	lv_obj_t *restartScr = lv_obj_create(NULL);
	lv_obj_set_size(restartScr, TFT_WIDTH, TFT_HEIGHT);
	lv_obj_t *restartMsg = lv_msgbox_create(restartScr);
	lv_obj_set_width(restartMsg,TFT_WIDTH - 20);
	lv_obj_set_align(restartMsg,LV_ALIGN_CENTER);
	lv_obj_set_style_text_font(restartMsg, fontDefault, 0);
	lv_obj_t *labelText = lv_msgbox_get_content(restartMsg);
	lv_obj_set_style_text_align(labelText, LV_TEXT_ALIGN_CENTER, 0);
	lv_msgbox_add_text(restartMsg, LV_SYMBOL_WARNING " This device will restart shortly");
	lv_screen_load(restartScr);
	lv_timer_t *restartTimer;
	restartTimer = lv_timer_create(restartTimerCb, 3000, NULL);
	lv_timer_reset(restartTimer);
}

/**
 * @brief Show message dialog
 *
 * @param symbol LVGL symbol font
 * @param message Message
 */
void showMsg(const char* symbol, const char* message)
{
	msgDialog = lv_msgbox_create(lv_scr_act());
	lv_obj_set_width(msgDialog,TFT_WIDTH);
	lv_obj_set_align(msgDialog,LV_ALIGN_CENTER);
	lv_obj_set_style_text_font(msgDialog, fontDefault, 0);
	lv_obj_t *labelText = lv_msgbox_get_content(msgDialog);
	lv_obj_set_style_text_align(labelText, LV_TEXT_ALIGN_CENTER, 0);
	char msg[100];
	snprintf(msg, sizeof(msg), "%s %s", symbol, message);
	lv_msgbox_add_text(msgDialog, msg);
	lv_obj_invalidate(msgDialog);
	lv_refr_now(display);
}

/**
 * @brief Close message dialog.
 *
 */
void closeMsg()
{
	lv_obj_del(msgDialog);
}