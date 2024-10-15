/**
 * @file deviceSettingsScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Device Settings Screen
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#ifndef DEVICESETTINGSCR_HPP
#define DEVICESETTINGSCR_HPP

#include "globalGuiDef.h"
#include "gps.hpp"
#include "settings.hpp"
#include "lvglSetup.hpp"

static lv_obj_t *deviceSettingsOptions;

static void deviceSettingsEvent(lv_event_t *event);
void createDeviceSettingsScr();

#endif
