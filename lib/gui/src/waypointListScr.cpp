/**
 * @file waypointListScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Waypoint list screen
 * @version 0.2.0
 * @date 2025-04
 */

#include "waypointListScr.hpp"

extern Maps mapView;
extern Storage storage;
extern wayPoint loadWpt;

lv_obj_t *listWaypointScreen; // Add Waypoint Screen

/**
 * @brief Way point list event
 * 
 */
void waypointListEvent(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(event);
  uint32_t row;
  uint32_t col;

  if (code == LV_EVENT_LONG_PRESSED)
  {
    lv_table_get_selected_cell(obj, &row, &col);

    if (row != 0)
    {
      String sel = String(lv_table_get_cell_value(obj, row, col));
      String wptName = sel.substring(6,sel.length());

      if (!sel.isEmpty())
      {
        GPXParser gpx(wptFile);
        loadWpt = gpx.getWaypointInfo(wptName);

        switch (wptAction)
        {
          case WPT_LOAD:
            lv_img_set_src(arrowNav, &navup);        

            if (loadWpt.lat != 0 && loadWpt.lon != 0)
            {
              lv_obj_clear_flag(navTile, LV_OBJ_FLAG_HIDDEN);

              lv_label_set_text_fmt(latNav, "%s", latFormatString(loadWpt.lat));
              lv_label_set_text_fmt(lonNav, "%s", lonFormatString(loadWpt.lon));
              lv_label_set_text_fmt(nameNav, "%s", loadWpt.name);

              mapView.setWaypoint(loadWpt.lat, loadWpt.lon);

              lv_obj_send_event(mapTile, LV_EVENT_REFRESH, NULL);
            }
            else
              lv_obj_add_flag(navTile, LV_OBJ_FLAG_HIDDEN);

            loadMainScreen();
            break;
          case WPT_EDIT:
            isMainScreen = false;
            mapView.redrawMap = false;

            lv_textarea_set_text(waypointName, loadWpt.name);
            isScreenRotated = false;
            lv_obj_set_width(waypointName, tft.width() - 10);
            updateWaypointPos();
            lv_screen_load(waypointScreen);
            break;
          case WPT_DEL:
            gpx.deleteWaypoint(loadWpt.name);
            loadMainScreen();
            break;
          default:
            break;
        }
      }
    }
    else if (row == 0)
    {
      lv_obj_add_flag(navTile, LV_OBJ_FLAG_HIDDEN);
      loadMainScreen();
    }
  }
}

/**
 * @brief Create List Waypoint Screen
 *
 */
void createWaypointListScreen()
{
  listWaypointScreen = lv_table_create(NULL);
  lv_obj_set_size(listWaypointScreen, TFT_WIDTH, TFT_HEIGHT);
  lv_table_set_cell_value(listWaypointScreen, 0, 0, LV_SYMBOL_LEFT " Waypoints");
  lv_table_set_column_width(listWaypointScreen, 0, TFT_WIDTH);
  lv_obj_add_event_cb(listWaypointScreen, waypointListEvent, LV_EVENT_ALL, NULL);
  lv_obj_set_style_pad_ver(listWaypointScreen, 15, LV_PART_ITEMS);
  lv_obj_set_style_border_width(listWaypointScreen, 1, LV_PART_ITEMS);
  lv_obj_set_style_border_color(listWaypointScreen, lv_color_hex(0x303030), LV_PART_ITEMS);
  lv_obj_set_style_border_side(listWaypointScreen, LV_BORDER_SIDE_BOTTOM, LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(listWaypointScreen, lv_color_hex(0x303030), LV_PART_ITEMS | LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(listWaypointScreen, LV_OPA_100, LV_PART_ITEMS | LV_STATE_PRESSED);
}

/**
 * @brief Update List Waypoint Screen from file
 *
 */
void updateWaypointListScreen()
{
  lv_obj_clean(listWaypointScreen);
  lv_table_set_row_count(listWaypointScreen, 1);
  isMainScreen = false;

  GPXParser gpx(wptFile);

  uint16_t totalWpt = 1;
  std::vector<std::string> names = gpx.getWaypointList();
  for (const std::string& name : names) 
  {
    lv_table_set_cell_value_fmt(listWaypointScreen, totalWpt, 0, LV_SYMBOL_GPS " - %s", name.c_str());
    totalWpt++;
  }
}