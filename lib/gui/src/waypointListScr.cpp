/**
 * @file waypointListScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Waypoint list screen
 * @version 0.1.8
 * @date 2024-06
 */

#include "waypointListScr.hpp"

std::string wptContent;

lv_obj_t *listWaypointScreen;  // Add Waypoint Screen

/**
 * @brief Way point list event
 * 
 */
void waypointListEvent(lv_event_t * event)
{

    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(event);
    uint32_t row;
    uint32_t col;

    if(code == LV_EVENT_LONG_PRESSED)
    {  
        lv_table_get_selected_cell(obj, &row, &col);

        if ( row!= 0 )
        {
            String sel = String(lv_table_get_cell_value(obj, row, col));
            if (!sel.isEmpty())
            {
                std::string wptSelected = sel.substring(6,sel.length()).c_str();
                std::regex wptGet("lat=\"([^\"]+)\"\\s+lon=\"([^\"]+)\">\\s*<name>([^<]*?)</name>");
                std::smatch wptFound;
                std::string::const_iterator wptSearch(wptContent.cbegin());

                while (std::regex_search(wptSearch, wptContent.cend(), wptFound, wptGet))
                {
                    std::string lat = wptFound[1].str();  
                    std::string lon = wptFound[2].str();  
                    std::string name = wptFound[3].str(); 

                    if ( name == wptSelected )
                    {
                        //addWpt.name = (char*)name.c_str();
                        loadWpt.name = new char[name.size() + 1];
                        std::strcpy(loadWpt.name, name.c_str());
                        loadWpt.lat = std::stod(lat);
                        loadWpt.lon = std::stod(lon);
                        log_i("Waypoint: %s %s %s",name.c_str(), lat.c_str(), lon.c_str());
                        break;
                    }
                    wptSearch = wptFound.suffix().first; 
                }
                
                // TODO Call Edit , Delete screens (identify origin)
                lv_obj_clear_flag(navTile,LV_OBJ_FLAG_HIDDEN);
                updateNavScreen();
                loadMainScreen();
            }
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
    lv_table_set_cell_value(listWaypointScreen, 0, 0, " Waypoints");
    lv_table_set_column_width(listWaypointScreen,0,TFT_WIDTH);
    lv_obj_add_event_cb(listWaypointScreen, waypointListEvent, LV_EVENT_ALL, NULL);
    lv_obj_set_style_pad_ver(listWaypointScreen, 15, LV_PART_ITEMS);
    lv_obj_set_style_border_width(listWaypointScreen,1, LV_PART_ITEMS);
    lv_obj_set_style_border_color(listWaypointScreen, lv_color_hex(0x303030), LV_PART_ITEMS);
    lv_obj_set_style_border_side(listWaypointScreen, LV_BORDER_SIDE_BOTTOM, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(listWaypointScreen, lv_color_hex(0x303030), LV_PART_ITEMS | LV_STATE_PRESSED );
    lv_obj_set_style_bg_opa(listWaypointScreen,LV_OPA_100, LV_PART_ITEMS | LV_STATE_PRESSED );
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

    acquireSdSPI();

    File wayPointFile = SD.open(wptFile);

    if (!wayPointFile)
        log_e("Waypoint file not found");
    else
    {
        wptContent = "";
        while (wayPointFile.available()) 
            wptContent += (char)wayPointFile.read();

        log_i("Waypoint file found");
        wayPointFile.close();
    
        std::regex wptName("<name>([^<]*?)</name>");
        std::smatch wptFound;
        std::string::const_iterator wptSearch(wptContent.cbegin());

        int totalWpt = 1;

        while (std::regex_search(wptSearch, wptContent.cend(), wptFound, wptName))
        {
            lv_table_set_cell_value_fmt(listWaypointScreen, totalWpt, 0, LV_SYMBOL_GPS " - %s", wptFound[1].str().c_str());
            totalWpt++;
            wptSearch = wptFound.suffix().first;  
        }
    }

    releaseSdSPI();
}