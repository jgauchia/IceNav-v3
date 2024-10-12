/**
 * @file loadWaypoint.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Load Waypoint functions
 * @version 0.1.8_Alpha
 * @date 2024-10
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

    std::regex wptGet("lat=\"([^\"]+)\"\\s+lon=\"([^\"]+)\">\\s*<name>(" + wptSelected + ")</name>");

    std::smatch wptFound;
    std::string::const_iterator wptSearch(wptContent.cbegin());

    loadWpt.name = new char [wptSelected.length()+1];
    loadWpt.lat = 0;
    loadWpt.lon = 0;

    while (std::regex_search(wptSearch, wptContent.cend(), wptFound, wptGet))
    {
        std::string lat = wptFound[1].str();  
        std::string lon = wptFound[2].str();  
 
        std::strcpy (loadWpt.name, wptSelected.c_str());

        loadWpt.lat = std::stod(lat);
        loadWpt.lon = std::stod(lon);
        log_i("Waypoint: %s %s %s",wptSelected.c_str(), lat.c_str(), lon.c_str());
        log_i("%s",loadWpt.name);

        wptSearch = wptFound.suffix().first; 
    }
 }