/**
 * @file lvglFuncs.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL custom functions
 * @version 0.3.3
 * @date 2026-10
 */

#pragma once

#include "tft.hpp"
#include "globalGuiDef.h"

void objHideCursor(_lv_obj_t *obj);
void objSelect(_lv_obj_t *obj);
void objUnselect(_lv_obj_t *obj);
void restartTimerCb(lv_timer_t *timer);
void showRestartScr();
void showMsg(const char* symbol, const char* message, bool refresh = true);
void closeMsg();

/**
 * @brief Queue an LVGL async callback without racing lv_timer_handler
 *
 * @details lv_async_call() allocates memory and inserts a node in the LVGL timer list without
 *          taking any lock, so it must not run while the GUI task is inside lv_timer_handler().
 *          The mutex is taken only when the current task does not hold it already, which is the
 *          case for callbacks running inside the GUI task; that keeps timer callbacks from
 *          self-deadlocking on the non-recursive mutex.
 *
 * @param cb Async callback to schedule.
 * @return true when the callback was queued, false when it was dropped.
 */
bool lvglAsyncCall(lv_async_cb_t cb);
