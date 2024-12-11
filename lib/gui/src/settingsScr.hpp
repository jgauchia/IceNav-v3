/**
 * @file settingsScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Settings Screen
 * @version 0.1.9
 * @date 2024-12
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
