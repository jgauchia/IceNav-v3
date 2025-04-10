/**
 * @file lvglFuncs.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL custom functions
 * @version 0.2.0
 * @date 2025-04
 */

#ifndef LVGLFUNCS_HPP
#define LVGLFUNCS_HPP

#include "tft.hpp"
#include "globalGuiDef.h"

void objHideCursor(_lv_obj_t *obj);
void objSelect(_lv_obj_t *obj);
void objUnselect(_lv_obj_t *obj);
void restartTimerCb(lv_timer_t *timer);
void showRestartScr();

#endif
