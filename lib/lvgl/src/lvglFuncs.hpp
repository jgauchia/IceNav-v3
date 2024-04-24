/**
 * @file lvglFuncs.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL custom functions
 * @version 0.1.8
 * @date 2024-04
 */

#ifndef LVGLFUNCS_HPP
#define LVGLFUNCS_HPP

#include "lvgl.h"

void objHideCursor(_lv_obj_t *obj);
void objSelect(_lv_obj_t *obj);
void objUnselect(_lv_obj_t *obj);

#endif