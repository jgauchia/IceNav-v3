/**
 * @file lvglSetup.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL Screen implementation
 * @version 0.1.8
 * @date 2024-04
 */

#ifndef LVGLSETUP_HPP
#define LVGLSETUP_HPP

#include <lvgl.h>

#include "lvglFuncs.hpp"
#include "lvglSpiffsFs.hpp"

#include "globalGuiDef.h"
#include "splashScr.hpp"
#include "searchSatScr.hpp"
#include "notifyBar.hpp"
#include "mainScr.hpp"
#include "buttonBar.hpp"
#include "settingsScr.hpp"
#include "deviceSettingsScr.hpp"
#include "mapSettingsScr.hpp"

/**
 * @brief Default display driver definition
 *
 */
static lv_display_t *display;
#define DRAW_BUF_SIZE (TFT_WIDTH * TFT_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
// static uint32_t *drawBuf = (uint32_t *)ps_malloc(TFT_WIDTH * TFT_HEIGHT / 10 * (LV_COLOR_DEPTH / 8));
static uint32_t drawBuf[DRAW_BUF_SIZE / 4];

void displayFlush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void touchRead(lv_indev_t *indev_driver, lv_indev_data_t *data);
void initLVGL();
void loadMainScreen();

#endif