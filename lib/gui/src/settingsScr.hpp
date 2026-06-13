/**
 * @file settingsScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Settings Screen
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include "globalGuiDef.h"
#include "mainScr.hpp"
#include "searchSatScr.hpp"
#include "compass.hpp"
#include "styles.hpp"

void loadMainScreen();

static void back(lv_event_t *event);
static void touchCalib(lv_event_t *event);
static void compassCalib(lv_event_t *event);
static void mapSettings(lv_event_t *event);
static void deviceSettings(lv_event_t *event);
#if defined(BATT_PIN) || defined(BME280) || defined(ENABLE_IMU) || defined(ENABLE_COMPASS)
static void sensorInfo(lv_event_t *event);
#endif

static lv_obj_t *settingsButtons; /**< Object holding all settings screen buttons */
void createSettingsScr();
