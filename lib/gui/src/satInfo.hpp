/**
 * @file satInfo.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Satellites info screen functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

#ifndef SATINFO_HPP
#define SATINFO_HPP

#include <lvgl.h>
#include "tft.hpp"
#include "gpsMath.hpp"
#include "gps.hpp"
#include <stdint.h>

struct SatPos // Structure to store satellite position in constellation map
{
  uint16_t x;
  uint16_t y;
};

extern SatPos satPos; // Satellite position X,Y in constellation map

extern TFT_eSprite spriteSNR1;       // Sprite for snr GPS Satellite Labels
extern TFT_eSprite spriteSNR2;       // Sprite for snr GPS Satellite Labels
extern TFT_eSprite spriteSat;        // Sprite for satellite position in map
extern TFT_eSprite constelSprite;    // Sprite for Satellite Constellation

extern lv_obj_t *satelliteBar1;               // Satellite Signal Graphics Bars
extern lv_obj_t *satelliteBar2;               // Satellite Signal Graphics Bars
extern lv_chart_series_t *satelliteBarSerie1; // Satellite Signal Graphics Bars
extern lv_chart_series_t *satelliteBarSerie2; // Satellite Signal Graphics Bars

SatPos getSatPos(uint8_t elev, uint16_t azim);
void deleteSatInfoSprites();
void createConstelSprite(TFT_eSprite &spr);
void createSatSprite(TFT_eSprite &spr);
void createSNRSprite(TFT_eSprite &spr);
void drawSNRBar(lv_obj_t *bar, lv_chart_series_t *barSer, uint8_t id, uint8_t satNum, uint8_t snr, TFT_eSprite &spr);
void clearSatInView();
void fillSatInView();
#endif
