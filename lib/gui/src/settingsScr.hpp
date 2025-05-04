/**
 * @file settingsScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Settings Screen
 * @version 0.2.0_alpha
 * @date 2025-01
 */

#ifndef SETTINGSSCR_HPP
#define SETTINGSSCR_HPP

#include "globalGuiDef.h"
#include "mainScr.hpp"
#include "searchSatScr.hpp"

void loadMainScreen();

static void back(lv_event_t *event);
static void touchCalib(lv_event_t *event);
static void compassCalib(lv_event_t *event);
static void mapSettings(lv_event_t *event);
static void deviceSettings(lv_event_t *event);

static lv_obj_t *settingsButtons;
void createSettingsScr();

#endif
