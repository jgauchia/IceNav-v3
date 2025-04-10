/**
 * @file lvglSetup.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL Screen implementation
 * @version 0.2.0
 * @date 2025-04
 */

#ifndef LVGLSETUP_HPP
#define LVGLSETUP_HPP

#define LV_TICK_PERIOD_MS 5

#include "lvgl_private.h"
#include "globalGpxDef.h"
#include "tasks.hpp"
#include "cli.hpp"
#include "bme.hpp"
#include "maps.hpp"
#include "firmUpgrade.hpp"
#include "splashScr.hpp"
#include "notifyBar.hpp"
#include "settingsScr.hpp"
#include "deviceSettingsScr.hpp"
#include "mapSettingsScr.hpp"


/**
 * @brief Default display driver definition
 *
 */
static uint32_t objectColor = 0x303030; 

void IRAM_ATTR displayFlush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void IRAM_ATTR touchRead(lv_indev_t *indev_driver, lv_indev_data_t *data);
#ifdef TDECK_ESP32S3 
	void IRAM_ATTR keypadRead(lv_indev_t *indev_driver, lv_indev_data_t *data);
	uint32_t keypadGetKey();
#endif
#ifdef POWER_SAVE
	static const uint16_t longPressTime = 1000; // Long press time 
	void IRAM_ATTR gpioRead(lv_indev_t *indev_driver, lv_indev_data_t *data);
	void gpioLongEvent(lv_event_t *event);
	void gpioClickEvent(lv_event_t *event);
	uint8_t gpioGetBut();
#endif
void applyModifyTheme(lv_theme_t *th, lv_obj_t *obj);
void modifyTheme();
void lv_tick_task(void *arg);
void initLVGL();
void loadMainScreen();

#endif
