/**
 * @file lvglSetup.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL Screen implementation
 * @version 0.1.8
 * @date 2024-06
 */

#ifndef LVGLSETUP_HPP
#define LVGLSETUP_HPP

#include <lvgl.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LV_TICK_PERIOD_MS 5

#include "lvglFuncs.hpp"
#include "lvglSpiffsFs.hpp"

#include "globalGuiDef.h"
#include "splashScr.hpp"
#include "searchSatScr.hpp"
#include "notifyBar.hpp"
#include "mainScr.hpp"
#include "buttonBar.hpp"
#include "settingsScr.hpp"
#include "deviceSettingsScr.hpp"
#include "mapSettingsScr.hpp"

/**
 * @brief Default display driver definition
 *
 */
static lv_display_t *display;
static uint32_t objectColor = 0x303030; 

void displayFlush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void touchRead(lv_indev_t *indev_driver, lv_indev_data_t *data);
void applyModifyTheme(lv_theme_t *th, lv_obj_t *obj);
void modifyTheme();
void lv_tick_task(void *arg);
void initLVGL();
void loadMainScreen();

#endif
