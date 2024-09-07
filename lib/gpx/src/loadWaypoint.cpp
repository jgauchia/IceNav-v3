/**
 * @file loadWaypoint.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Load Waypoint functions
 * @version 0.1.8_Alpha
 * @date 2024-09
 */

 #include "loadWaypoint.hpp"

/**
 * @brief Load Selected Waypoint.
 * 
 * @param wpt -> Selected waypoint name
 */
 void loadWptFile(String wpt)
 {
    std::string wptSelected = wpt.substring(6,wpt.length()).c_str();
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
 }