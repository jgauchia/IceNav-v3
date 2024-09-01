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

wayPoint selectedWpt = {0, 0, 0, "", "", "", "", "", "", 0, 0, 0, 0};

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
                log_i("%s",wptSelected.c_str());
                std::regex wptGet("<wpt\\s+lat=\"([^\"]+)\"\\s+lon=\"([^\"]+)\">\\s*<name>(.*?)</name>\\s*</wpt>");
                std::smatch wptFound;
                std::string::const_iterator wptSearch(wptContent.cbegin());

                while (std::regex_search(wptSearch, wptContent.cend(), wptFound, wptGet))
                {
                    std::string lat = wptFound[1].str();  
                    std::string lon = wptFound[2].str();  
                    std::string name = wptFound[3].str(); 

                    if ( name == wptSelected )
                    {
                        selectedWpt.name = (char*)name.c_str();
                        selectedWpt.lat = std::stod(lat);
                        selectedWpt.lon = std::stod(lon);
                        log_i("Waypoint: %s %f %f",selectedWpt.name, selectedWpt.lat, selectedWpt.lon);
                    }
                    wptSearch = wptFound.suffix().first; 
                }
                
                // TODO Call Edit , Delete screens
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
    lv_table_set_cell_value(listWaypointScreen, 0, 0, "Waypoints");
    lv_obj_add_event_cb(listWaypointScreen, waypointListEvent, LV_EVENT_ALL, NULL);
    lv_obj_set_style_pad_ver(listWaypointScreen, 10, LV_PART_ITEMS);
}

/**
 * @brief Update List Waypoint Screen from file
 *
 */
void updateWaypointListScreen()
{
    // if (lv_table_get_row_count(listWaypointScreen) != 1)
    // for (int i = 1; i < lv_table_get_row_count(listWaypointScreen); i++)
    //     lv_table_set_cell_value(listWaypointScreen, i, 0, "");
    // //lv_table_set_cell_value(listWaypointScreen, 0, 0, "Waypoints");
    // lv_table_set_row_count(listWaypointScreen, 1);
    lv_obj_clean(listWaypointScreen);
    lv_table_set_row_count(listWaypointScreen, 1);

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