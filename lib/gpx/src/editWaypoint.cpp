/**
 * @file editWaypoint.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Edit Waypoint functions
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

 #include "editWaypoint.hpp"

/**
 * @brief Edit Waypoint Name
 * 
 * @param oldName -> Waypoint old name
 * @param newName -> Waypoint new name
 */
 void editWaypointName(char* oldName, char *newName)
 {
    std::string wptOldName = std::string((char*)oldName);
    std::string wptNewName = std::string((char*)newName);

    std::regex findWptName("(\\s<name>" + wptOldName + "</name>)");
    std::string replaceWptName = " <name>" + wptNewName + "</name>";

    wptContent = std::regex_replace(wptContent,findWptName,replaceWptName);


    size_t fileSize = wptContent.length();

    acquireSdSPI();

    File wayPointFile = SD.open(wptFile, FILE_WRITE);

    if (!wayPointFile)
      log_e("Error updating waypoint file");
    else
    {
      wayPointFile.seek(0);
      wayPointFile.write((uint8_t*)wptContent.c_str(), fileSize);
      wayPointFile.close();
      log_i("Waypoint file updated");
    }

    releaseSdSPI();

 }