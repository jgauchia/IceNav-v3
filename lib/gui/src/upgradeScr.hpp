/**
 * @file upgradeScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL Firmware upgrade messages
 * @version 0.2.3
 * @date 2025-06
 */

#pragma once

#include "lvgl.h"
#include "firmUpgrade.hpp"
#include "globalGuiDef.h"

extern lv_obj_t *msgUpgrade;      /**< Upgrade message object */
extern lv_obj_t *msgUprgdText;    /**< Text label inside the upgrade message */
extern lv_obj_t *btnMsgBack;      /**< Back button inside the message dialog */
extern lv_obj_t *btnMsgUpgrade;   /**< Upgrade action button inside the message dialog */
extern lv_obj_t *contMeter;       /**< Meter container object */

void msgBackEvent(lv_event_t *event);
void msgUpgrdEvent(lv_event_t *event);
void createMsgUpgrade();