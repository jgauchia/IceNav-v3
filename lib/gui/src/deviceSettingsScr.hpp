/**
 * @file deviceSettingsScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Device Settings Screen
 * @version 0.1.8
 * @date 2024-04
 */

#ifndef DEVICESETTINGSCR_HPP
#define DEVICESETTINGSCR_HPP

#include "globalGuiDef.h"
#include "gps.hpp"
#include "settings.hpp"

static lv_obj_t *deviceSettingsOptions;

static void deviceSettingsBack(lv_event_t *event);
static void setGpsSpeed(lv_event_t *event);
static void setGpsUpdateRate(lv_event_t *event);
void createDeviceSettingsScr();

#endif