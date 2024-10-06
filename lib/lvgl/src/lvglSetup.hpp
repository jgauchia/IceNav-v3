/**
 * @file lvglSetup.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL Screen implementation
 * @version 0.1.8_Alpha
 * @date 2024-09
 */

#ifndef LVGLSETUP_HPP
#define LVGLSETUP_HPP

#include <lvgl.h>

#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#ifdef TDECK_ESP32S3 
    #include "Wire.h"
#endif

#define LV_TICK_PERIOD_MS 5

#include "globalGuiDef.h"
#include "splashScr.hpp"
#include "searchSatScr.hpp"
#include "notifyBar.hpp"
#include "widgets.hpp"
#include "mainScr.hpp"
#include "buttonBar.hpp"
#include "settingsScr.hpp"
#include "deviceSettingsScr.hpp"
#include "mapSettingsScr.hpp"
#include "waypointScr.hpp"

/**
 * @brief Default display driver definition
 *
 */
//extern lv_display_t *display;
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
