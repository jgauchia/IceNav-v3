/**
 * @file map.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Map screen events
 * @version 0.1.6
 * @date 2023-06-14
 */

void generate_render_map();
void generate_vector_map();

static const char *map_scale[] = {"5000 Km", "2500 Km", "1500 Km", "700 Km", "350 Km",
                                  "150 Km", "100 Km", "40 Km", "20 Km", "10 Km", "5 Km",
                                  "2,5 Km", "1,5 Km", "700 m", "350 m", "150 m", "80 m",
                                  "40 m", "20 m", "10 m"};

/**
 * @brief Update zoom value
 *
 * @param event
 */
static void get_zoom_value(lv_event_t *event)
{
  lv_obj_t *screen = lv_event_get_current_target(event);
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
  if (act_tile == MAP && is_main_screen)
  {
    switch (dir)
    {
    case LV_DIR_LEFT:
      break;
    case LV_DIR_RIGHT:
      break;
    case LV_DIR_TOP:
      if (!vector_map)
      {
        if (zoom >= MIN_ZOOM && zoom < MAX_ZOOM)
          zoom++;
      }
      else
      {
        zoom--;
        position_moved = true;
        if (zoom < 1)
        {
          zoom = 1;
          position_moved = false;
        }
        if (zoom > 4)
        {
          zoom = 4;
          position_moved = false;
        }
      }
      lv_event_send(map_tile, LV_EVENT_REFRESH, NULL);
      break;
    case LV_DIR_BOTTOM:
      if (!vector_map)
      {
        if (zoom <= MAX_ZOOM && zoom > MIN_ZOOM)
          zoom--;
      }
      else
      {
        zoom++;
        position_moved = true;
        if (zoom < 1)
        {
          zoom = 1;
          position_moved = false;
        }
        if (zoom > 4)
        {
          zoom = 4;
          position_moved = false;
        }
      }
      lv_event_send(map_tile, LV_EVENT_REFRESH, NULL);
      break;
    }
  }
}

/**
 * @brief Delete map screen sprites and release PSRAM
 *
 */
static void delete_map_scr_sprites()
{
  sprArrow.deleteSprite();
  map_rot.deleteSprite();
}

/**
 * @brief Create a map screen sprites
 *
 */
static void create_map_scr_sprites()
{
  // Map Sprite
  map_rot.createSprite(MAP_WIDTH, MAP_HEIGHT);
  map_rot.pushSprite(0, 27);
  // Arrow Sprite
  sprArrow.createSprite(16, 16);
  sprArrow.setColorDepth(16);
  sprArrow.pushImage(0, 0, 16, 16, (uint16_t *)navigation);
}

/**
 * @brief Draw map widgets
 *
 */
void draw_map_widgets()
{
  map_rot.setTextColor(TFT_WHITE, TFT_WHITE);

#ifdef ENABLE_COMPASS
  heading = get_heading();
  if (map_rotation)
    map_heading = get_heading();
  else
    map_heading = GPS.course.deg();
  if (show_map_compass)
  {
    map_rot.fillRectAlpha(TFT_WIDTH - 48, 0, 48, 48, 95, TFT_BLACK);
    map_rot.pushImageRotateZoom(TFT_WIDTH - 24, 24, 24, 24, 360 - heading, 1, 1, 48, 48, (uint16_t *)mini_compass, TFT_BLACK);
  }
#endif

  map_rot.fillRectAlpha(0, 0, 50, 32, 95, TFT_BLACK);
  map_rot.pushImage(0, 4, 24, 24, (uint16_t *)zoom_ico, TFT_BLACK);
  map_rot.drawNumber(zoom, 26, 8, &fonts::FreeSansBold9pt7b);

  if (show_map_speed)
  {
    map_rot.fillRectAlpha(0, 342, 70, 32, 95, TFT_BLACK);
    map_rot.pushImage(0, 346, 24, 24, (uint16_t *)speed_ico, TFT_BLACK);
    map_rot.drawNumber((uint16_t)GPS.speed.kmph(), 26, 350, &fonts::FreeSansBold9pt7b);
  }

  if (!vector_map)
    if (show_map_scale)
    {
      map_rot.fillRectAlpha(250, 342, 70, TFT_WIDTH - 245, 95, TFT_BLACK);
      map_rot.setTextSize(1);
      map_rot.drawFastHLine(255, 360, 60);
      map_rot.drawFastVLine(255, 355, 10);
      map_rot.drawFastVLine(315, 355, 10);
      map_rot.drawCenterString(map_scale[zoom], 285, 350);
    }
}

/**
 * @brief Update map event
 *
 * @param event
 */
static void update_map(lv_event_t *event)
{
  if (vector_map)
  {
    tft.startWrite();
    get_position(getLat(), getLon());

    if (position_moved)
    {
      map_rot.deleteSprite();
      map_rot.createSprite(MAP_WIDTH, MAP_HEIGHT);
      viewPort.setCenter(point);
      get_map_blocks(memBlocks, viewPort.bbox);
      //draw_map_widgets();
      generate_vector_map(viewPort, memBlocks, map_rot);
      //map_rot.pushSprite(0, 27);
      draw_map_widgets();
      // map_rot.pushSprite(0, 27);

      refresh_map = true;
      position_moved = false;
    }

    if (refresh_map)
    {
      //draw_map_widgets();
      map_rot.pushSprite(0, 27);
    }
    tft.endWrite();
  }
  else
    generate_render_map();
}
