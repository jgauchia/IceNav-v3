/**
 * @file satInfoScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Satellite info screen 
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include "globalGuiDef.h"
#include "gps.hpp"
#include "styles.hpp"

/**
 * @brief Satellite SV Colors
 *
 * Color definitions for active and inactive states of satellite systems in the UI.
 */
// GPS
#define GP_ACTIVE_COLOR     lv_color_hex(0x104828)  /**< Active color for GPS satellites. */
#define GP_INACTIVE_COLOR   lv_color_hex(0x229954)  /**< Inactive color for GPS satellites. */

// GLONASS
#define GL_ACTIVE_COLOR     lv_color_hex(0x11364d)  /**< Active color for GLONASS satellites. */
#define GL_INACTIVE_COLOR   lv_color_hex(0x2471a3)  /**< Inactive color for GLONASS satellites. */

// BEIDOU
#define BD_ACTIVE_COLOR     lv_color_hex(0x3b1c48)  /**< Active color for BEIDOU satellites. */
#define BD_INACTIVE_COLOR   lv_color_hex(0x7d3c98)  /**< Inactive color for BEIDOU satellites. */

/**
 * @brief Satellite Tracking Tile screen objects.
 *
 * @details Objects used in the satellite tracking tile 
 */
extern lv_obj_t *pdopLabel;          /**< Label for PDOP value. */
extern lv_obj_t *hdopLabel;          /**< Label for HDOP value. */
extern lv_obj_t *vdopLabel;          /**< Label for VDOP value. */
extern lv_obj_t *altLabel;           /**< Label for altitude value. */
extern lv_obj_t *constCanvas;        /**< Canvas for constellation visualization. */
extern lv_obj_t *satelliteBar;       /**< Chart object for satellite signal bar. */
extern lv_chart_series_t *satelliteBarSerie; /**< Series for satellite signal bar chart. */
extern lv_obj_t *constMsg;           /**< Label for constellation status messages. */

/**
 * @brief Satellite Constellation Layer Canvas Definition.
 *
 * @details Definitions for the color and layers used for drawing the satellite constellation on the canvas.
 */
#define CONSTEL_COLOR lv_color_hex(0x515a5a)    /**< Color used for the constellation canvas background. */
extern lv_layer_t canvasLayer;                  /**< Layer for the base canvas drawing. */
extern lv_layer_t satLayer;                     /**< Layer for satellite elements. */


void satelliteBarDrawEvent(lv_event_t *event);
void constSatEvent(lv_event_t *event);
void closeConstSatEvent(lv_event_t *event);
void createConstCanvas(_lv_obj_t *screen);
void satelliteScr(_lv_obj_t *screen);
void drawSatSNR();
void drawSatConst();
void drawSatSky();