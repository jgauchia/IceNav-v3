/**
 * @file satInfoScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Satellite info screen 
 * @version 0.1.9
 * @date 2024-12
 */

#ifndef SATINFOSCR_HPP
#define SATINFOSCR_HPP

#include "globalGuiDef.h"
#include "gps.hpp"

/**
 * @brief Satellite SV Colors
 *
 */
// GPS
#define GP_ACTIVE_COLOR lv_color_hex(0x104828)
#define GP_INACTIVE_COLOR lv_color_hex(0x229954)
// GLONASS
#define GL_ACTIVE_COLOR lv_color_hex(0x11364d)
#define GL_INACTIVE_COLOR lv_color_hex(0x2471a3)
// BEIDOU
#define BD_ACTIVE_COLOR lv_color_hex(0x3b1c48)
#define BD_INACTIVE_COLOR lv_color_hex(0x7d3c98)

/**
 * @brief Satellite Tracking Tile screen objects
 *
 */
extern lv_obj_t *pdopLabel;
extern lv_obj_t *hdopLabel;
extern lv_obj_t *vdopLabel;
extern lv_obj_t *altLabel;
extern lv_obj_t *constCanvas;
extern lv_obj_t *satelliteBar;               
extern lv_chart_series_t *satelliteBarSerie; 
extern lv_obj_t *constMsg;

/**
 * @brief Satellite Constellation Layer Canvas Definition
 *
 */
 #define CONSTEL_COLOR lv_color_hex(0x515a5a)
 extern lv_layer_t canvasLayer;
 extern lv_layer_t satLayer;


void drawTextOnLayer(const char * text, lv_layer_t * layer, lv_point_t * p, lv_area_t * coords, lv_color_t color, const void * font, int16_t offset);
void satelliteBarDrawEvent(lv_event_t *event);
void constSatEvent(lv_event_t *event);
void closeConstSatEvent(lv_event_t *event);
void createConstCanvas(_lv_obj_t *screen);
void satelliteScr(_lv_obj_t *screen);
void drawSatSNR();
void drawSatConst();
void drawSatSky();
#endif