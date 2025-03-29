/**
 * @file editWaypoint.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Edit Waypoint functions
 * @version 0.2.0_alpha
 * @date 2025-03
 */

 #include "editWaypoint.hpp"
 extern Storage storage;

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

    FILE *wayPointFile = storage.open(wptFile, "w");

    if (!wayPointFile)
      log_e("Error updating waypoint file");
    else
    {
      storage.seek(wayPointFile, 0, SEEK_SET);
      storage.write(wayPointFile,(uint8_t*)wptContent.c_str(), fileSize);
      storage.close(wayPointFile);
      log_i("Waypoint file updated");
    }
 }