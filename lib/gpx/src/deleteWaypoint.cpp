/**
 * @file deleteWaypoint.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Delete Waypoint functions
 * @version 0.1.8_Alpha
 * @date 2024-09
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
    
    std::regex delWpt("(^<wpt[^>]*>\\s*<name>" + wptDelete + "</name>[\\s\\S]*?</wpt>$)");

    std::string result = std::regex_replace(wptContent, delWpt, "HOLA");

    log_i("%s",result.c_str());
}

    //  std::string wptOldName = std::string((char*)oldName);
    // std::string wptNewName = std::string((char*)newName);

    // std::regex findWptName("(\\s<name>" + wptOldName + "</name>)");
    // std::string replaceWptName = " <name>" + wptNewName + "</name>";

    // wptContent = std::regex_replace(wptContent,findWptName,replaceWptName);


    // size_t fileSize = wptContent.length();

    // acquireSdSPI();

    // File wayPointFile = SD.open(wptFile, FILE_WRITE);

    // if (!wayPointFile)
    //   log_e("Error updating waypoint file");
    // else
    // {
    //   wayPointFile.seek(0);
    //   wayPointFile.write((uint8_t*)wptContent.c_str(), fileSize);
    //   wayPointFile.close();
    //   log_i("Waypoint file updated");
    // }

    // releaseSdSPI();