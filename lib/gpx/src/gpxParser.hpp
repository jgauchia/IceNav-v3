/**
 * @file gpxParser.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPX Parser class
 * @version 0.2.1_alpha
 * @date 2025-04
 */

#ifndef GPXPARSER_HPP
#define GPXPARSER_HPP

#include <Arduino.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "tinyxml2.h"
#include "globalGpxDef.h"

class GPXParser
{
  public:
    GPXParser(const char* filePath);
    ~GPXParser();

    static std::map<std::string, std::vector<std::string>> getWaypointList(const std::string& folderPath);
    wayPoint getWaypointInfo(const char* name);
    bool addWaypoint(const wayPoint& wp);
    bool editWaypointName(const char* oldName, const char* newName);
    bool deleteWaypoint(const char* name);
  
    static std::map<std::string, std::vector<std::string>> getTrackList(const std::string& folderPath);
    bool deleteTrack(const char* name);
    
  private:
    std::string filePath;
};

#endif