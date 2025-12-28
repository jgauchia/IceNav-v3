/**
 * @file mapsVars.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Maps vars and structures
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include "bruj.h"
#include "compass.h"
#include "waypoint.h"
#include "navfinish.h"
#include "straight.h"
#include "slleft.h"
#include "slright.h"
#include "tleft.h"
#include "tright.h"
#include "uleft.h"
#include "uright.h"
#include "finish.h"
#include "outtrack.h"

#include "globalGpxDef.h"

static const char *mapVectorFolder PROGMEM = "/sdcard/VECTMAP/%u/%u/%u.bin";        /**< Vector Map Files Folder */
static const char *mapRenderFolder PROGMEM = "/sdcard/MAP/%u/%u/%u.png"; /**< Render Maps file folder */
static const char *noMapFile PROGMEM = "/spiffs/NOMAP.png";              /**< No map image file */
static const char *map_scale[] PROGMEM = {"5000 Km", "2500 Km", "1500 Km",
                                        "700 Km", "350 Km", "150 Km",
                                        "100 Km", "40 Km", "20 Km",
                                        "10 Km", "5 Km", "2,5 Km",
                                        "1,5 Km", "700 m", "350 m",
                                        "150 m", "80 m", "40 m",
                                        "20 m", "10 m"
                                        }; /**< Scale label for map */
