/**
 * @file deleteWaypoint.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Delete Waypoint functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

 #include "deleteWaypoint.hpp"

/**
 * @brief Delete Waypoint 
 * 
 * @param wpt -> Waypoint name
 */
void deleteWaypointName(char * wpt)
{
    std::string wptDelete = std::string((char*)wpt);
    
    std::regex delWpt("<wpt lat=\"([^\"]+)\"\\s+lon=\"([^\"]+)\">\\s*<name>(" + wptDelete + ")</name>\\s*</wpt>\\s*");

    std::string result = std::regex_replace(wptContent, delWpt, "");

    size_t fileSize = result.length();

    acquireSdSPI();

    File wayPointFile = SD.open(wptFile, FILE_WRITE);

    if (!wayPointFile)
      log_e("Error updating waypoint file");
    else
    {
      wayPointFile.seek(0);
      wayPointFile.write((uint8_t*)result.c_str(), fileSize);
      wayPointFile.close();
      log_i("Waypoint file updated");
    }

    releaseSdSPI();
}
