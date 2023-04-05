/**
 * @file map.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Map screen events
 * @version 0.1
 * @date 2022-11-20
 */

/**
 * @brief Zoom Levels and Default zoom
 *
 */
#define MIN_ZOOM 6
#define MAX_ZOOM 17
#define DEF_ZOOM 17
int zoom = DEF_ZOOM;

/**
 * @brief Old tile coordinates and zoom
 *
 */
// int tilex_old = 0;
// int tiley_old = 0;
// int zoom_old = 0;

MapTile CurrentMapTile;
MapTile OldMapTile;

/**
 * @brief Navitagion Arrow position on screen
 *
 */
ScreenCoord NavArrow_position;

/**
 * @brief Flag to indicate when tile map is found on SD
 *
 */
bool map_found = false;

/**
 * @brief Sprite for Navigation Arrow in map tile
 *
 */
TFT_eSprite sprArrow = TFT_eSprite(&tft);

/**
 * @brief Double Buffering Sprites for Map Tile
 * 
 */
TFT_eSprite map_spr = TFT_eSprite(&tft);
TFT_eSprite map_buf = TFT_eSprite(&tft);

/**
 * @brief Update zoom value
 *
 * @param event
 */
static void get_zoom_value(lv_event_t *event)
{
  lv_obj_t *screen = lv_event_get_current_target(event);
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
  if (act_tile == MAP)
  {
    switch (dir)
    {
    case LV_DIR_LEFT:
      break;
    case LV_DIR_RIGHT:
      break;
    case LV_DIR_TOP:
      if (zoom >= MIN_ZOOM && zoom < MAX_ZOOM)
      {
        zoom++;
        lv_label_set_text_fmt(zoom_label, "ZOOM: %2d", zoom);
        lv_event_send(map_tile, LV_EVENT_VALUE_CHANGED, NULL);
      }
      lv_indev_wait_release(lv_indev_get_act());
      break;
    case LV_DIR_BOTTOM:
      if (zoom <= MAX_ZOOM && zoom > MIN_ZOOM)
      {
        zoom--;
        lv_label_set_text_fmt(zoom_label, "ZOOM: %2d", zoom);
        lv_event_send(map_tile, LV_EVENT_VALUE_CHANGED, NULL);
      }
      lv_indev_wait_release(lv_indev_get_act());
      break;
    }
  }
}

/**
 * @brief Draw map event
 *
 * @param event
 */
static void draw_map(lv_event_t *event)
{
  if (!is_map_draw)
  {
    OldMapTile.zoom = 0;
    OldMapTile.tilex = 0;
    OldMapTile.tiley = 0;
    OldMapTile.file = ""; // TODO - Delete Warning
    is_map_draw = true;
    map_found = false;
  }
}

/**
 * @brief Update map event
 *
 * @param event
 */
static void update_map(lv_event_t *event)
{
  CurrentMapTile = get_map_tile(GPS.location.lng(), GPS.location.lat(), zoom);
  if (strcmp(CurrentMapTile.file, OldMapTile.file) != 0 || CurrentMapTile.zoom != OldMapTile.zoom || CurrentMapTile.tilex != OldMapTile.tilex || CurrentMapTile.tiley != OldMapTile.tiley)
  {
    OldMapTile.zoom = CurrentMapTile.zoom;
    OldMapTile.tilex = CurrentMapTile.tilex;
    OldMapTile.tiley = CurrentMapTile.tiley;
    OldMapTile.file = CurrentMapTile.file;

    map_spr.deleteSprite();
    map_spr.createSprite(256, 256);
    map_buf.deleteSprite();
    map_buf.createSprite(256, 256);

    map_found = map_spr.drawPngFile(SD, CurrentMapTile.file, 0, 0);
    map_buf.drawPngFile(SD, CurrentMapTile.file, 0, 0);
    map_spr.pushSprite(32, 64);
  }
  if (map_found)
  {
    uint8_t data[1800];
    sprArrow.deleteSprite();
    sprArrow.createSprite(16, 16);

    sprArrow.fillSprite(TFT_BLACK);
    sprArrow.pushImage(0, 0, 16, 16, (uint16_t *)navigation);

    NavArrow_position = coord_to_scr_pos(0, 0, GPS.location.lng(), GPS.location.lat(), zoom);

    map_buf.readRect(NavArrow_position.posx - 12, NavArrow_position.posy - 12, 24, 24, (uint16_t *)data);
    map_spr.setPivot(NavArrow_position.posx, NavArrow_position.posy);
    map_spr.pushImage(NavArrow_position.posx - 12, NavArrow_position.posy - 12, 24, 24, (uint16_t *)data);

    heading = read_compass();
    sprArrow.pushRotated(&map_spr, heading, TFT_BLACK);

    map_spr.pushSprite(32, 64);
  }
}
