/**
 * @file mapsVars.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Maps vars and structures
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include "../../images/src/bruj.h"
#include "../../images/src/compass.h"
#include "../../images/src/waypoint.h"
#include "../../images/src/navfinish.h"
#include "../../images/src/straight.h"
#include "../../images/src/slleft.h"
#include "../../images/src/slright.h"
#include "../../images/src/tleft.h"
#include "../../images/src/tright.h"
#include "../../images/src/uleft.h"
#include "../../images/src/uright.h"
#include "../../images/src/finish.h"
#include "../../images/src/outtrack.h"

#include "globalGpxDef.h"

static const char *mapRenderFolder = "/sdcard/MAP/%u/%u/%u.png"; /**< Render Maps file folder */
static const char *mapVectorFolder = "/sdcard/NAVMAP/%d/%d/%d.nav"; /**< Vector Maps file folder */
static const char *noMapFile = "/spiffs/NOMAP.png";              /**< No map image file */
static const char *map_scale[] = {"5000 Km", "2500 Km", "1500 Km",
                                        "700 Km", "350 Km", "150 Km",
                                        "100 Km", "40 Km", "20 Km",
                                        "10 Km", "5 Km", "2,5 Km",
                                        "1,5 Km", "700 m", "350 m",
                                        "150 m", "80 m", "40 m",
                                        "20 m", "10 m"
                                        }; /**< Scale label for map */
