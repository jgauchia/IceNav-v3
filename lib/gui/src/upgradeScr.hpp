/**
 * @file upgradeScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL Firmware upgrade messages
 * @version 0.2.1
 * @date 2025-05
 */

#pragma once

#include "lvgl.h"
#include "firmUpgrade.hpp"
#include "globalGuiDef.h"

extern lv_obj_t *msgUpgrade;
extern lv_obj_t *msgUprgdText;
extern lv_obj_t *btnMsgBack;
extern lv_obj_t *btnMsgUpgrade;
extern lv_obj_t *contMeter;

void msgBackEvent(lv_event_t *event);
void msgUpgrdEvent(lv_event_t *event);
void createMsgUpgrade();