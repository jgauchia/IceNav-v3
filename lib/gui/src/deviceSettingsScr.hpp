/**
 * @file deviceSettingsScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Device Settings Screen
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include "globalGuiDef.h"
#include "upgradeScr.hpp"
#include "lvglSetup.hpp"
#include "firmUpgrade.hpp"
#include "lvglFuncs.hpp"
#include "settings.hpp"

static lv_obj_t *deviceSettingsOptions; /**< Device settings options */

static void deviceSettingsEvent(lv_event_t *event);
static void brightnessEvent(lv_event_t *e);
static void upgradeEvent(lv_event_t *event);
void createDeviceSettingsScr();
