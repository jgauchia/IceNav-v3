/**
 * @file settingsScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Settings Screen
 * @version 0.2.3
 * @date 2025-06
 */

#include "settingsScr.hpp"

bool needReboot = false; /**< Flag to indicate if a system reboot is required */

extern Compass compass;

/**
 * @brief Back button event
 *
 * @details Handles the back button action
 *
 * @param event LVGL event pointer.
 */
static void back(lv_event_t *event)
{
	if (isSearchingSat)
	{
		lv_timer_pause(mainTimer);
		lv_screen_load(searchSatScreen);
	}
	else
		loadMainScreen();
}

/**
 * @brief Touch Calibration
 *
 * @details Handles the touch calibration event. 
 *
 * @param event LVGL event pointer.
 */
static void touchCalib(lv_event_t *event)
{
	repeatCalib = true;
	tft.fillScreen(TFT_BLACK);
	touchCalibrate();
	repeatCalib = false;
	isMainScreen = false;
	tft.fillScreen(TFT_BLACK);
	lv_screen_load(settingsScreen);
	lv_obj_invalidate(lv_scr_act());
}

/**
 * @brief Compass Calibration
 *
 * @details Handles the compass calibration event.
 *
 * @param event LVGL event pointer.
 */
#ifdef ENABLE_COMPASS
static void compassCalib(lv_event_t *event)
{
	tft.fillScreen(TFT_BLACK);
	compass.calibrate();
	tft.fillScreen(TFT_BLACK);
	isMainScreen = false;
	lv_screen_load(settingsScreen);
	lv_obj_invalidate(lv_scr_act());
}
#endif

/**
 * @brief Map Setting
 *
 * @details Handles the map settings event by loading the map settings screen.
 *
 * @param event LVGL event pointer.
 */
static void mapSettings(lv_event_t *event)
{
 	 lv_screen_load(mapSettingsScreen);
}

/**
 * @brief Device Settings
 *
 * @details Handles the device settings event by loading the device settings screen.
 *
 * @param event LVGL event pointer.
 */
static void deviceSettings(lv_event_t *event)
{
 	 lv_screen_load(deviceSettingsScreen);
}

/**
 * @brief Create Settings screen
 *
 * @details Creates the settings screen 
 */
void createSettingsScr()
{
	// Settings Screen
	settingsScreen = lv_obj_create(NULL);
	settingsButtons = lv_obj_create(settingsScreen);
	lv_obj_set_size(settingsButtons, TFT_WIDTH, TFT_HEIGHT);
	lv_obj_set_flex_align(settingsButtons, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_row(settingsButtons, 20, 0);
	lv_obj_set_flex_flow(settingsButtons, LV_FLEX_FLOW_COLUMN);
	static lv_style_t styleSettings;
	lv_style_init(&styleSettings);
	lv_style_set_bg_opa(&styleSettings, LV_OPA_0);
	lv_style_set_border_opa(&styleSettings, LV_OPA_0);
	lv_obj_add_style(settingsButtons, &styleSettings, LV_PART_MAIN);

	lv_obj_t *btnLabel;
	lv_obj_t *btn;

	#ifdef ENABLE_COMPASS
	// Compass Calibration
	btn = lv_btn_create(settingsButtons);
	lv_obj_set_size(btn, TFT_WIDTH - 30, 40 * scale);
	btnLabel = lv_label_create(btn);
	lv_obj_set_style_text_font(btnLabel, fontLarge, 0);
	lv_label_set_text_static(btnLabel, "Compass Calibration");
	lv_obj_center(btnLabel);
	lv_obj_add_event_cb(btn, compassCalib, LV_EVENT_CLICKED, NULL);
	#endif

	#ifdef TOUCH_INPUT
	// Touch Calibration
	btn = lv_btn_create(settingsButtons);
	lv_obj_set_size(btn, TFT_WIDTH - 30, 40 * scale);
	btnLabel = lv_label_create(btn);
	lv_obj_set_style_text_font(btnLabel, fontLarge, 0);
	lv_label_set_text_static(btnLabel, "Touch Calibration");
	lv_obj_center(btnLabel);
	lv_obj_add_event_cb(btn, touchCalib, LV_EVENT_CLICKED, NULL);
	#endif

	// Map Settings
	btn = lv_btn_create(settingsButtons);
	lv_obj_set_size(btn, TFT_WIDTH - 30, 40 * scale);
	btnLabel = lv_label_create(btn);
	lv_obj_set_style_text_font(btnLabel, fontLarge, 0);
	lv_label_set_text_static(btnLabel, "Map Settings");
	lv_obj_center(btnLabel);
	lv_obj_add_event_cb(btn, mapSettings, LV_EVENT_CLICKED, NULL);

	// Device Settings
	btn = lv_btn_create(settingsButtons);
	lv_obj_set_size(btn, TFT_WIDTH - 30, 40 * scale);
	btnLabel = lv_label_create(btn);
	lv_obj_set_style_text_font(btnLabel, fontLarge, 0);
	lv_label_set_text_static(btnLabel, "Device Settings");
	lv_obj_center(btnLabel);
	lv_obj_add_event_cb(btn, deviceSettings, LV_EVENT_CLICKED, NULL);

	// Back button
	btn = lv_btn_create(settingsButtons);
	lv_obj_set_size(btn, TFT_WIDTH - 30, 40 * scale);
	btnLabel = lv_label_create(btn);
	lv_obj_set_style_text_font(btnLabel, fontLarge, 0);
	lv_label_set_text_static(btnLabel, "Back");
	lv_obj_center(btnLabel);
	lv_obj_add_event_cb(btn, back, LV_EVENT_CLICKED, NULL);
}
