/**
 * @file mapsDrawFunc.h
 * @brief  Extra Draw functions for maps
 * @version 0.1.8
 * @date 2024-06
 */

#ifndef MAPSDRAWFUNC_H
#define MAPSDRAWFUNC_H

#include "globalMapsDef.h"

// Images
#include "bruj.c"
#include "navigation.c"
#include "compass.c"
#include "zoom.c"
#include "speed.c"

// Scale for map
static const char *map_scale[] PROGMEM = {"5000 Km", "2500 Km", "1500 Km",
                                          "700 Km", "350 Km", "150 Km",
                                          "100 Km", "40 Km", "20 Km",
                                          "10 Km", "5 Km", "2,5 Km",
                                          "1,5 Km", "700 m", "350 m",
                                          "150 m", "80 m", "40 m",
                                          "20 m", "10 m"};

static uint16_t *mapPtr;

/**
 * @brief Delete map screen sprites and release PSRAM
 *
 */
static void deleteMapScrSprites()
{
  sprArrow.deleteSprite();
  mapSprite.deleteSprite();
}

/**
 * @brief Create a map screen sprites
 *
 */
static void createMapScrSprites()
{
  // Map Sprite
  mapPtr = (uint16_t*)mapSprite.createSprite(MAP_WIDTH, MAP_HEIGHT);
  // Arrow Sprite
  sprArrow.createSprite(16, 16);
  sprArrow.setColorDepth(16);
  sprArrow.pushImage(0, 0, 16, 16, (uint16_t *)navigation);
}

/**
 * @brief Draw map widgets
 *
 */
static void drawMapWidgets()
{
  mapSprite.setTextColor(TFT_WHITE, TFT_WHITE);

  #ifdef ENABLE_COMPASS
  if (isMapRotation)
    mapHeading = heading;
  else
    mapHeading = GPS.course.deg();
  if (showMapCompass)
  {
    mapSprite.fillRectAlpha(MAP_WIDTH - 48, 0, 48, 48, 95, TFT_BLACK);
    if (isCompassRot)
      mapSprite.pushImageRotateZoom(MAP_WIDTH - 24, 24, 24, 24, 360 - heading, 1, 1, 48, 48, (uint16_t *)mini_compass, TFT_BLACK);
    else
      mapSprite.pushImage(MAP_WIDTH - 48, 0, 48, 48, (uint16_t *)mini_compass, TFT_BLACK);
  }
  #endif

  mapSprite.fillRectAlpha(0, 0, 50, 32, 95, TFT_BLACK);
  mapSprite.pushImage(0, 4, 24, 24, (uint16_t *)zoom_ico, TFT_BLACK);
  mapSprite.drawNumber(zoom, 26, 8, &fonts::FreeSansBold9pt7b);

  if (showMapSpeed)
  {
    mapSprite.fillRectAlpha(0, MAP_HEIGHT - 32, 70, 32, 95, TFT_BLACK);
    mapSprite.pushImage(0, MAP_HEIGHT - 28, 24, 24, (uint16_t *)speed_ico, TFT_BLACK);
    mapSprite.drawNumber((uint16_t)GPS.speed.kmph(), 26, MAP_HEIGHT - 24 , &fonts::FreeSansBold9pt7b);
  }

  if (!isVectorMap)
    if (showMapScale)
    {
      mapSprite.fillRectAlpha(MAP_WIDTH - 70, MAP_HEIGHT - 32 , 70, MAP_WIDTH - 75, 95, TFT_BLACK);
      mapSprite.setTextSize(1);
      mapSprite.drawFastHLine(MAP_WIDTH - 65 , MAP_HEIGHT - 14 , 60);
      mapSprite.drawFastVLine(MAP_WIDTH - 65  , MAP_HEIGHT - 19 , 10);
      mapSprite.drawFastVLine(MAP_WIDTH - 5, MAP_HEIGHT - 19 , 10);
      mapSprite.drawCenterString(map_scale[zoom], MAP_WIDTH - 35 , MAP_HEIGHT - 24);
    }
}

/**
 * @brief Display Map
 *
 * @param tileSize -> Tile Size to center map
 */
static void displayMap(uint16_t tileSize)
{
  mapSprite.pushSprite(0, 27); 

  if (isMapFound)
  {
    navArrowPosition = coord2ScreenPos(getLon(), getLat(), zoom, tileSize);

    if (tileSize == RENDER_TILE_SIZE)
      mapTempSprite.setPivot(tileSize + navArrowPosition.posX, tileSize + navArrowPosition.posY);

    if (tileSize == VECTOR_TILE_SIZE)
      mapTempSprite.setPivot(tileSize , tileSize );

    #ifdef ENABLE_COMPASS

    if (isMapRotation)
      mapHeading = heading;
    else
      mapHeading = GPS.course.deg();

    #else 

    mapHeading = GPS.course.deg();

    #endif

    mapTempSprite.pushRotated(&mapSprite, 360 - mapHeading, TFT_TRANSPARENT);
    //mapTempSprite.pushRotated(&mapSprite, 0, TFT_TRANSPARENT);
   
    sprArrow.pushRotated(&mapSprite, 0, TFT_BLACK);
    drawMapWidgets();
  }
  else
    mapTempSprite.pushSprite(&mapSprite, 0, 0, TFT_TRANSPARENT);
}

/**
 * @brief crop buffer image
 *
 * @param origBuff -> Original buffer
 * @param cropBuff -> Buffer cropped
 * @param xOffset -> X offset
 * @param yOffset -> Y offset
 * @param width -> Width crop
 * @param height -> Height crop
 */
static void cropImage(const uint16_t *origBuff, uint16_t *cropBuff, int xOffset, int yOffset, int width, int height)
{
  for (int y = 0; y < height; y++)
  {
    int yOrigin = y + yOffset;
    int xOrigin = xOffset;
    memcpy(cropBuff + y * width, origBuff + yOrigin * MAP_WIDTH + xOrigin, width * sizeof(uint16_t));
  }
}

#endif
