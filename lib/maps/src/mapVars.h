/**
 * @file mapVars.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Maps variables and structures
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

static const char *mapRenderFolder = "/sdcard/MAP/%u/%u/%u.png"; /**< Render Maps file folder */
static const char *mapVectorFolder = "/sdcard/NAVMAP/Z%u.nav"; /**< Vector Maps file folder */
static const char *noMapFile = "/spiffs/gfx/NOMAP.png";              /**< No map image file */
static const char *map_scale[] = {"5000 Km", "2500 Km", "1500 Km",
                                        "700 Km", "350 Km", "150 Km",
                                        "100 Km", "40 Km", "20 Km",
                                        "10 Km", "5 Km", "2,5 Km",
                                        "1,5 Km", "700 m", "350 m",
                                        "150 m", "80 m", "40 m",
                                        "20 m", "10 m"
                                        }; /**< Scale label for map */

/** PNG tile load order: spiral from center outward. */
static constexpr int8_t PNG_SPIRAL_ORDER[9][2] =
    {{1,1},{0,1},{1,0},{2,1},{1,2},{0,0},{2,0},{0,2},{2,2}};

/** NAV tile enqueue order: corners first, then edges, center last. */
static constexpr int8_t NAV_SPIRAL_ORDER[9][2] =
    {{0,0},{2,0},{0,2},{2,2},{0,1},{1,0},{2,1},{1,2},{1,1}};