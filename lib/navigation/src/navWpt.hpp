/**
 * @file navWpt.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Waypoint Navigation Functions
 * @version 0.1.8_Alpha
 * @date 2024-08
 */

#ifndef NAVWPT_HPP
#define NAVWPT_HPP

// #include "lvgl.h"
// #include "lvglFuncs.hpp"
// #include "tft.hpp"
// #include "gps."
// #include "globalGuiDef.h"
// #include "gpsMath.hpp"
// #include "mainScr.hpp"

#include <Arduino.h>
#include "tft.hpp"
#include "compass.hpp"
#include "settings.hpp"
#include "globalMapsDef.h"
#include "mapsDrawFunc.h"

void updateNavScreen();

#endif