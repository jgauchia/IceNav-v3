/**
 * @file map.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Map screen events
 * @version 0.1.8
 * @date 2024-04
 */

void generateRenderMap();
void generateVectorMap();

static const char *map_scale[] = {"5000 Km", "2500 Km", "1500 Km", "700 Km", "350 Km",
                                  "150 Km", "100 Km", "40 Km", "20 Km", "10 Km", "5 Km",
                                  "2,5 Km", "1,5 Km", "700 m", "350 m", "150 m", "80 m",
                                  "40 m", "20 m", "10 m"};

/**
 * @brief Update zoom value
 *
 * @param event
 */
static void getZoomValue(lv_event_t *event)
{
  lv_obj_t *screen = (lv_obj_t*)lv_event_get_current_target(event);
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
  if (activeTile == MAP && isMainScreen)
  {
    switch (dir)
    {
    case LV_DIR_LEFT:
      break;
    case LV_DIR_RIGHT:
      break;
    case LV_DIR_TOP:
      if (!isVectorMap)
      {
        if (zoom >= MIN_ZOOM && zoom < MAX_ZOOM)
          zoom++;
      }
      else
      {
        zoom--;
        isPosMoved = true;
        if (zoom < 1)
        {
          zoom = 1;
          isPosMoved = false;
        }
        if (zoom > 4)
        {
          zoom = 4;
          isPosMoved = false;
        }
      }
      lv_obj_send_event(mapTile, LV_EVENT_REFRESH, NULL);
      break;
    case LV_DIR_BOTTOM:
      if (!isVectorMap)
      {
        if (zoom <= MAX_ZOOM && zoom > MIN_ZOOM)
          zoom--;
      }
      else
      {
        zoom++;
        isPosMoved = true;
        if (zoom < 1)
        {
          zoom = 1;
          isPosMoved = false;
        }
        if (zoom > 4)
        {
          zoom = 4;
          isPosMoved = false;
        }
      }
      lv_obj_send_event(mapTile, LV_EVENT_REFRESH, NULL);
      break;
    }
  }
}

/**
 * @brief Delete map screen sprites and release PSRAM
 *
 */
static void deleteMapScrSprites()
{
  sprArrow.deleteSprite();
  mapRotSprite.deleteSprite();
}

/**
 * @brief Create a map screen sprites
 *
 */
static void createMapScrSprites()
{
  // Map Sprite
  mapRotSprite.createSprite(MAP_WIDTH, MAP_HEIGHT);
  mapRotSprite.pushSprite(0, 27);
  // Arrow Sprite
  sprArrow.createSprite(16, 16);
  sprArrow.setColorDepth(16);
  sprArrow.pushImage(0, 0, 16, 16, (uint16_t *)navigation);
}

/**
 * @brief Draw map widgets
 *
 */
void drawMapWidgets()
{
  mapRotSprite.setTextColor(TFT_WHITE, TFT_WHITE);

#ifdef ENABLE_COMPASS
  heading = getHeading();
  if (isMapRotation)
    mapHeading = getHeading();
  else
    mapHeading = GPS.course.deg();
  if (showMapCompass)
  {
    mapRotSprite.fillRectAlpha(TFT_WIDTH - 48, 0, 48, 48, 95, TFT_BLACK);
    mapRotSprite.pushImageRotateZoom(TFT_WIDTH - 24, 24, 24, 24, 360 - heading, 1, 1, 48, 48, (uint16_t *)mini_compass, TFT_BLACK);
  }
#endif

  mapRotSprite.fillRectAlpha(0, 0, 50, 32, 95, TFT_BLACK);
  mapRotSprite.pushImage(0, 4, 24, 24, (uint16_t *)zoom_ico, TFT_BLACK);
  mapRotSprite.drawNumber(zoom, 26, 8, &fonts::FreeSansBold9pt7b);

  if (showMapSpeed)
  {
    mapRotSprite.fillRectAlpha(0, 342, 70, 32, 95, TFT_BLACK);
    mapRotSprite.pushImage(0, 346, 24, 24, (uint16_t *)speed_ico, TFT_BLACK);
    mapRotSprite.drawNumber((uint16_t)GPS.speed.kmph(), 26, 350, &fonts::FreeSansBold9pt7b);
  }

  if (!isVectorMap)
    if (showMapScale)
    {
      mapRotSprite.fillRectAlpha(250, 342, 70, TFT_WIDTH - 245, 95, TFT_BLACK);
      mapRotSprite.setTextSize(1);
      mapRotSprite.drawFastHLine(255, 360, 60);
      mapRotSprite.drawFastVLine(255, 355, 10);
      mapRotSprite.drawFastVLine(315, 355, 10);
      mapRotSprite.drawCenterString(map_scale[zoom], 285, 350);
    }
}

/**
 * @brief Update map event
 *
 * @param event
 */
static void updateMap(lv_event_t *event)
{
  if (isVectorMap)
  {
    if (tft.getStartCount() == 0)
      tft.startWrite();
    getPosition(getLat(), getLon());

    if (isPosMoved)
    {
      mapRotSprite.deleteSprite();
      mapRotSprite.createSprite(MAP_WIDTH, MAP_HEIGHT);
      viewPort.setCenter(point);
      getMapBlocks(viewPort.bbox, memCache);
      generateVectorMap(viewPort, memCache, mapRotSprite);
      refreshMap = true;
      isPosMoved = false;
    }

    if (refreshMap)
    {
      mapRotSprite.pushSprite(0, 27, TFT_TRANSPARENT);
      drawMapWidgets();
    }
    if (tft.getStartCount() > 0)
      tft.endWrite();
  }
  else
    generateRenderMap();
}
