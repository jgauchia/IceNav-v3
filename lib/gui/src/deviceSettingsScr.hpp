/**
 * @file deviceSettingsScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Device Settings Screen
 * @version 0.1.9_alpha
 * @date 2024-11
 */

#ifndef DEVICESETTINGSCR_HPP
#define DEVICESETTINGSCR_HPP

#include "globalGuiDef.h"
#include "lvglSetup.hpp"

static lv_obj_t *deviceSettingsOptions;

static void deviceSettingsEvent(lv_event_t *event);
void createDeviceSettingsScr();

#endif
