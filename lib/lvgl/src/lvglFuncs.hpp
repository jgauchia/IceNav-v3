/**
 * @file lvglFuncs.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL custom functions
 * @version 0.1.8
 * @date 2024-05
 */

#ifndef LVGLFUNCS_HPP
#define LVGLFUNCS_HPP

#include <Arduino.h>
#include "lvgl.h"

void objHideCursor(_lv_obj_t *obj);
void objSelect(_lv_obj_t *obj);
void objUnselect(_lv_obj_t *obj);
void restartTimerCb(lv_timer_t *timer);
void showRestartScr();

#endif