/**
 * @file gpxScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - GPX list screen
 * @version 0.2.1
 * @date 2025-05
 */

#include "gpxScr.hpp"
#include "esp_log.h"

extern Maps mapView;
extern Storage storage;
extern wayPoint loadWpt;
extern bool isWaypointOpt;
extern bool isTrackOpt;
bool gpxWaypoint = false;
bool gpxTrack = false;
extern std::vector<wayPoint> trackData;

lv_obj_t *listGPXScreen; // Add Waypoint Screen

static const char* TAG PROGMEM = "GPX List Screen";

/**
 * @brief Way point list event
 * 
 */
void gpxListEvent(lv_event_t *event)
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
      String gpxName = sel.substring(6,sel.length());
      String gpxFile = String(lv_table_get_cell_value(obj, row, col+1));

      if (gpxWaypoint)
        gpxFileFolder = String(wptFolder) + "/" + gpxFile;
      else if (gpxTrack)
        gpxFileFolder = String(trkFolder) + "/" + gpxFile;

      GPXParser gpx(gpxFileFolder.c_str());
      
      if (!sel.isEmpty())
      {
        switch (gpxAction)
        {
          case GPX_LOAD:
            if (gpxWaypoint)
            {
              loadWpt = gpx.getWaypointInfo(gpxName.c_str());
              LV_IMG_DECLARE(navup);
              lv_img_set_src(arrowNav, &navup);        

              if (loadWpt.lat != 0 && loadWpt.lon != 0)
              {
                lv_obj_clear_flag(navTile, LV_OBJ_FLAG_HIDDEN);

                lv_label_set_text_fmt(latNav, "%s", latFormatString(loadWpt.lat));
                lv_label_set_text_fmt(lonNav, "%s", lonFormatString(loadWpt.lon));
                lv_label_set_text_fmt(nameNav, "%s", loadWpt.name);

                mapView.setWaypoint(loadWpt.lat, loadWpt.lon);
                mapView.updateMap();

                lv_obj_send_event(mapTile, LV_EVENT_REFRESH, NULL);
              }
              else
                lv_obj_add_flag(navTile, LV_OBJ_FLAG_HIDDEN);
            }

            if (gpxTrack)
            {
              gpx.loadTrack(trackData);
              mapView.updateMap();
              lv_obj_send_event(mapTile, LV_EVENT_REFRESH, NULL);
            }
            
            loadMainScreen();
            break;
          case GPX_EDIT:
            isMainScreen = false;
            mapView.redrawMap = false;

            if (gpxWaypoint)
            {
              loadWpt = gpx.getWaypointInfo(gpxName.c_str());
              lv_textarea_set_text(gpxTagValue, loadWpt.name);
              lv_label_set_text_static(gpxTag, LV_SYMBOL_LEFT " Waypoint Name:");
              lv_obj_clear_flag(labelLat, LV_OBJ_FLAG_HIDDEN);
              lv_obj_clear_flag(labelLatValue, LV_OBJ_FLAG_HIDDEN);
              lv_obj_clear_flag(labelLon, LV_OBJ_FLAG_HIDDEN);
              lv_obj_clear_flag(labelLonValue, LV_OBJ_FLAG_HIDDEN);
            }

            if (gpxTrack)
            {
              loadWpt.name = strdup(gpxName.c_str());
              lv_textarea_set_text(gpxTagValue, loadWpt.name);
              lv_label_set_text_static(gpxTag, LV_SYMBOL_LEFT " Track Name:");
              lv_obj_add_flag(labelLat, LV_OBJ_FLAG_HIDDEN);
              lv_obj_add_flag(labelLatValue, LV_OBJ_FLAG_HIDDEN);
              lv_obj_add_flag(labelLon, LV_OBJ_FLAG_HIDDEN);
              lv_obj_add_flag(labelLonValue, LV_OBJ_FLAG_HIDDEN);
            }

            isScreenRotated = false;
            lv_obj_set_width(gpxTagValue, tft.width() - 10);
            updateWaypoint(gpxAction);
            lv_screen_load(gpxDetailScreen);

            break;
          case GPX_DEL:
            if (gpxWaypoint)
              gpx.deleteTagByName(gpxWaypointTag, gpxName.c_str());
            if (gpxTrack)
              gpx.deleteTagByName(gpxTrackTag, gpxName.c_str());

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
void createGpxListScreen()
{
  listGPXScreen = lv_table_create(NULL);
  lv_table_set_col_cnt(listGPXScreen, 2);
  lv_table_set_column_width(listGPXScreen,1,400);
  lv_obj_set_size(listGPXScreen, TFT_WIDTH, TFT_HEIGHT);
  lv_table_set_cell_value(listGPXScreen, 0, 0, LV_SYMBOL_LEFT " Waypoints");
  lv_table_set_cell_value(listGPXScreen, 0, 1, LV_SYMBOL_FILE " File");
  lv_table_set_column_width(listGPXScreen, 0, TFT_WIDTH);
  lv_obj_add_event_cb(listGPXScreen, gpxListEvent, LV_EVENT_ALL, NULL);
  lv_obj_set_style_pad_ver(listGPXScreen, 15, LV_PART_ITEMS);
  lv_obj_set_style_border_width(listGPXScreen, 1, LV_PART_ITEMS);
  lv_obj_set_style_border_color(listGPXScreen, lv_color_hex(0x303030), LV_PART_ITEMS);
  lv_obj_set_style_border_side(listGPXScreen, LV_BORDER_SIDE_BOTTOM, LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(listGPXScreen, lv_color_hex(0x303030), LV_PART_ITEMS | LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(listGPXScreen, LV_OPA_100, LV_PART_ITEMS | LV_STATE_PRESSED);
}

/**
 * @brief Update List Waypoint Screen from file
 *
 */
void updateGpxListScreen()
{
  lv_obj_clean(listGPXScreen);
  lv_table_set_row_count(listGPXScreen, 1);
  isMainScreen = false;

  if (isWaypointOpt)
  {
    gpxWaypoint = true;
    gpxTrack = false;
    uint16_t totalGpx = 1;
    std::map<std::string, std::vector<std::string>> waypointByFile = GPXParser::getTagElementList(gpxWaypointTag, gpxNameElem, wptFolder);

    for (std::map<std::string, std::vector<std::string>>::const_iterator it = waypointByFile.begin(); it != waypointByFile.end(); ++it)
    {
      const std::string& fileName = it->first;
      const std::vector<std::string>& waypointNames = it->second;

      for (const std::string& gpxTagValue : waypointNames)
      {
        lv_table_set_cell_value_fmt(listGPXScreen, totalGpx, 0, LV_SYMBOL_GPS " - %s", gpxTagValue.c_str());
        lv_table_set_cell_value_fmt(listGPXScreen, totalGpx, 1, "%s", fileName.c_str());
        totalGpx++;
      }
    }
  } 

  if (isTrackOpt)
  {
    gpxWaypoint = false;
    gpxTrack = true;
    uint16_t totalGpx = 1;
    std::map<std::string, std::vector<std::string>> tracksByFile = GPXParser::getTagElementList(gpxTrackTag, gpxNameElem, trkFolder);

    for (std::map<std::string, std::vector<std::string>>::const_iterator it = tracksByFile.begin(); it != tracksByFile.end(); ++it)
    {
      const std::string& fileName = it->first;
      const std::vector<std::string>& trackNames = it->second;

      for (const std::string& trackName : trackNames)
      {
        lv_table_set_cell_value_fmt(listGPXScreen, totalGpx, 0, LV_SYMBOL_SHUFFLE " - %s", trackName.c_str());
        lv_table_set_cell_value_fmt(listGPXScreen, totalGpx, 1, "%s", fileName.c_str());
        totalGpx++;
      }
    }
  }
}