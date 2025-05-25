/**
 * @file gpxParser.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPX Parser class
 * @version 0.2.1
 * @date 2025-05
 */

#include "gpxParser.hpp"

/**
 * @brief Helper function to format double values to a specific number of decimal places

 * @param value value in double format
 * @param precision set precision (decimals)
 * @return std::string Formatted value 
 */
std::string formatDouble(double value, int precision) 
{
  std::ostringstream out;
  out << std::fixed << std::setprecision(precision) << value;
  return out.str();
}

GPXParser::GPXParser(const char* filePath) : filePath(filePath) {}
GPXParser::GPXParser() : filePath("") {}
GPXParser::~GPXParser() {}

/**
 * @brief Retrieve tag elements value list from all GPX files in a specified folder.
 *
 * @param tag 
 * @param element
 * @param folderPath Path to the folder containing GPX files.
 * @return std::map<std::string, std::vector<std::string>> Map where the key is the file name and the value is a vector of tag element value.
 */
std::map<std::string, std::vector<std::string>> GPXParser::getTagElementList(const char* tag, const char* element, const std::string& folderPath)
{
  std::map<std::string, std::vector<std::string>> elementsByFile;

  DIR* dir = opendir(folderPath.c_str());
  if (!dir)
  {
    ESP_LOGE(TAGGPX, "Failed to open folder: %s", folderPath.c_str());
    return elementsByFile;
  }

  struct dirent* entry;
  while ((entry = readdir(dir)) != nullptr)
  {
    if (entry->d_type == DT_REG)
    {
      std::string fileName = entry->d_name;

      if (fileName.size() >= 4 && fileName.substr(fileName.size() - 4) == ".gpx")
      {
        std::string filePath = folderPath + "/" + fileName;

        GPXParser parser(filePath.c_str());
        std::vector<std::string> elementValue;

        tinyxml2::XMLDocument doc;
        tinyxml2::XMLError result = doc.LoadFile(filePath.c_str());
        if (result == tinyxml2::XML_SUCCESS)
        {
          tinyxml2::XMLElement* root = doc.RootElement();
          if (root)
          {
            for (tinyxml2::XMLElement* Tag = root->FirstChildElement(tag); Tag!= nullptr; Tag = Tag->NextSiblingElement(tag))
            {
              tinyxml2::XMLElement* valueElement = Tag->FirstChildElement(element);
              if (valueElement)
              {
                const char* value = valueElement->GetText();
                if (value)
                  elementValue.push_back(value);
              }
            }
          }
          else
            ESP_LOGE(TAGGPX, "Failed to get root element in file: %s", filePath.c_str());
        }
        else
          ESP_LOGE(TAGGPX, "Failed to load GPX file: %s", filePath.c_str());

        elementsByFile[fileName] = elementValue;
      }
    }
  }

  closedir(dir);
  return elementsByFile;
}
 
/**
 * @brief Delete a tag from the GPX file with specified name value
 *
 * @param tag
 * @param name The name value to delete 
 * @return true if the waypoint was deleted successfully, false otherwise
 */
bool GPXParser::deleteTagByName(const char* tag, const char* name)
{
  tinyxml2::XMLDocument doc;
  tinyxml2::XMLError result = doc.LoadFile(filePath.c_str());
  if (result != tinyxml2::XML_SUCCESS) 
  {
    ESP_LOGE(TAGGPX, "Failed to load file: %s", filePath.c_str());
    return false;
  }

  tinyxml2::XMLElement* root = doc.RootElement();
  if (!root) 
  {
    ESP_LOGE(TAGGPX, "Failed to get root element in file: %s", filePath.c_str());
    return false;
  }

  for (tinyxml2::XMLElement* Tag = root->FirstChildElement(tag); Tag != nullptr; Tag = Tag->NextSiblingElement(tag)) 
  {
    tinyxml2::XMLElement* nameElement = Tag->FirstChildElement(gpxNameElem);
    if (nameElement && strcmp(nameElement->GetText(), name) == 0) 
    {
      root->DeleteChild(Tag);
      result = doc.SaveFile(filePath.c_str());
      if (result != tinyxml2::XML_SUCCESS)
      {
        ESP_LOGE(TAGGPX, "Failed to save file: %s", filePath.c_str());
        return false;
      }

      // if (!root->FirstChildElement(gpxWaypointTag)) 
      // {
      //   if (!root->FirstChildElement(gpxTrackTag)) 
      //   {
      //     if (remove(filePath.c_str()) != 0)
      //     {
      //       ESP_LOGE(TAGGPX, "Failed to delete file: %s", filePath.c_str());
      //       return false;
      //     }
      //     ESP_LOGI(TAGGPX, "File %s deleted as it had no waypoints or tracks left", filePath.c_str());
      //   }
      //   else
      //   {
      //     ESP_LOGI(TAGGPX, "File %s not deleted as it contains tracks but no waypoints", filePath.c_str());
      //   }
      // }
      return true;
    }
  }

  ESP_LOGE(TAGGPX, "Tag with name %s not found in file: %s", name, filePath.c_str());
  return false;
}

/**
 * @brief Retrieve waypoint details for a given name from a GPX file
 *
 * @param name Waypoint name to search for
 * @return wayPoint Struct containing waypoint details
 */
wayPoint GPXParser::getWaypointInfo(const char* name) 
{
  wayPoint wp = {0};

  tinyxml2::XMLDocument doc;
  tinyxml2::XMLError result = doc.LoadFile(filePath.c_str());
  if (result != tinyxml2::XML_SUCCESS)
  {
    ESP_LOGE(TAGGPX, "Failed to load file: %s", filePath.c_str());
    return wp;
  }

  tinyxml2::XMLElement* root = doc.RootElement();
  if (!root) 
  {
    ESP_LOGE(TAGGPX, "Failed to get root element in file: %s", filePath.c_str());
    return wp;
  }

  tinyxml2::XMLElement* wpt = nullptr;
  for (wpt = root->FirstChildElement(gpxWaypointTag); wpt != nullptr; wpt = wpt->NextSiblingElement(gpxWaypointTag)) 
  {
    tinyxml2::XMLElement* nameElement = wpt->FirstChildElement(gpxNameElem);
    if (nameElement && strcmp(nameElement->GetText(), name) == 0) 
    {
      wp.name = strdup(nameElement->GetText());
      break;
    }
  }

  if (wpt)
  {
    wpt->QueryDoubleAttribute(gpxLatElem, &wp.lat);
    wpt->QueryDoubleAttribute(gpxLonElem, &wp.lon);

    tinyxml2::XMLElement* element = nullptr;

    element = wpt->FirstChildElement(gpxEleElem);
    if (element) 
      wp.ele = static_cast<float>(element->DoubleText());

    element = wpt->FirstChildElement(gpxTimeElem);
    if (element)
      wp.time = strdup(element->GetText());

    element = wpt->FirstChildElement(gpxDescElem);
    if (element) 
      wp.desc = strdup(element->GetText());

    element = wpt->FirstChildElement(gpxSrcElem);
    if (element) 
      wp.src = strdup(element->GetText());

    element = wpt->FirstChildElement(gpxSymElem);
    if (element) 
      wp.sym = strdup(element->GetText());

    element = wpt->FirstChildElement(gpxTypeElem);
    if (element) 
      wp.type = strdup(element->GetText());

    element = wpt->FirstChildElement(gpxSatElem);
    if (element) 
      wp.sat = static_cast<uint8_t>(element->UnsignedText());

    element = wpt->FirstChildElement(gpxHdopElem);
    if (element) 
      wp.hdop = static_cast<float>(element->DoubleText());

    element = wpt->FirstChildElement(gpxVdopElem);
    if (element) 
      wp.vdop = static_cast<float>(element->DoubleText());

    element = wpt->FirstChildElement(gpxPdopElem);
    if (element) 
      wp.pdop = static_cast<float>(element->DoubleText());
  }
  else
      ESP_LOGE(TAGGPX, "Waypoint with name %s not found in file: %s", name, filePath.c_str());

  return wp;
}
 
/**
 * @brief Add a new waypoint to the GPX file
 *
 * @param wp Waypoint struct containing the waypoint details
 * @return true if the waypoint was added successfully, false otherwise
 */
bool GPXParser::addWaypoint(const wayPoint& wp)
{
  time_t tUTCwpt = time(NULL);
  struct tm UTCwpt_tm;
  struct tm *tmUTCwpt = gmtime_r(&tUTCwpt, &UTCwpt_tm);
  char textFmt[30];
  strftime(textFmt, sizeof(textFmt), "%Y-%m-%dT%H:%M:%SZ", tmUTCwpt);

  tinyxml2::XMLDocument doc;
  tinyxml2::XMLError result = doc.LoadFile(filePath.c_str());
  if (result != tinyxml2::XML_SUCCESS) 
  {
    ESP_LOGE(TAGGPX, "Failed to load file: %s", filePath.c_str());
    return false;
  }

  tinyxml2::XMLElement* root = doc.RootElement();
  if (!root)  
  {
    ESP_LOGE(TAGGPX, "Failed to get root element in file: %s", filePath.c_str());
    return false;
  }

  tinyxml2::XMLElement* newWpt = doc.NewElement(gpxWaypointTag);
  newWpt->SetAttribute(gpxLatElem, formatDouble(wp.lat, 6).c_str());
  newWpt->SetAttribute(gpxLonElem, formatDouble(wp.lon, 6).c_str());

  tinyxml2::XMLElement* element = nullptr;

  element = doc.NewElement(gpxEleElem);
  element->SetText(wp.ele);
  newWpt->InsertEndChild(element);

  element = doc.NewElement(gpxTimeElem);
  element->SetText(textFmt);
  newWpt->InsertEndChild(element);

  element = doc.NewElement(gpxNameElem);
  element->SetText(wp.name ? wp.name : "");
  newWpt->InsertEndChild(element);

  // element = doc.NewElement(gpxDescElem);
  // element->SetText(wp.desc ? wp.desc : "");
  // newWpt->InsertEndChild(element);

  element = doc.NewElement(gpxSrcElem);
  element->SetText(wp.src ? wp.src : "IceNav");
  newWpt->InsertEndChild(element);

  // element = doc.NewElement(gpxSymElem);
  // element->SetText(wp.sym ? wp.sym : "");
  // newWpt->InsertEndChild(element);

  // element = doc.NewElement(gpxTypeElem);
  // element->SetText(wp.type ? wp.type : "");
  // newWpt->InsertEndChild(element);

  element = doc.NewElement(gpxSatElem);
  element->SetText(wp.sat);
  newWpt->InsertEndChild(element);

  element = doc.NewElement(gpxHdopElem);
  element->SetText(wp.hdop);
  newWpt->InsertEndChild(element);

  element = doc.NewElement(gpxVdopElem);
  element->SetText(wp.vdop);
  newWpt->InsertEndChild(element);

  element = doc.NewElement(gpxPdopElem);
  element->SetText(wp.pdop);
  newWpt->InsertEndChild(element);

  tinyxml2::XMLElement* lastWpt = root->LastChildElement(gpxWaypointTag);
  if (lastWpt) 
    root->InsertAfterChild(lastWpt, newWpt);
  else
    root->InsertFirstChild(newWpt);

  result = doc.SaveFile(filePath.c_str());
  if (result != tinyxml2::XML_SUCCESS)
  {
    ESP_LOGE(TAGGPX, "Failed to save file: %s", filePath.c_str());
    return false;
  }
  return result == tinyxml2::XML_SUCCESS;
}

/**
 * @brief Load GPX track data and store coordinates from '<trk>' structure.
 *
 * @param trackData Vector to store track points (each point is a wayPoint).
 * @return true if the track data was loaded successfully, false otherwise.
 */
bool GPXParser::loadTrack(std::vector<wayPoint>& trackData)
{
  tinyxml2::XMLDocument doc;
  tinyxml2::XMLError result = doc.LoadFile(filePath.c_str());
  if (result != tinyxml2::XML_SUCCESS)
  {
    ESP_LOGE(TAGGPX, "Failed to load file: %s", filePath.c_str());
    return false;
  }

  tinyxml2::XMLElement* root = doc.RootElement();
  if (!root)
  {
    ESP_LOGE(TAGGPX, "Failed to get root element in file: %s", filePath.c_str());
    return false;
  }

  // Iterate through <trk> elements
  for (tinyxml2::XMLElement* trk = root->FirstChildElement(gpxTrackTag); trk != nullptr; trk = trk->NextSiblingElement(gpxTrackTag))
  {
    // Iterate through <trkseg> elements
    for (tinyxml2::XMLElement* trkseg = trk->FirstChildElement("trkseg"); trkseg != nullptr; trkseg = trkseg->NextSiblingElement("trkseg"))
    {
      // Iterate through <trkpt> elements
      for (tinyxml2::XMLElement* trkpt = trkseg->FirstChildElement("trkpt"); trkpt != nullptr; trkpt = trkpt->NextSiblingElement("trkpt"))
      {
        wayPoint point = {0};

        // Extract latitude and longitude
        trkpt->QueryDoubleAttribute(gpxLatElem, &point.lat);
        trkpt->QueryDoubleAttribute(gpxLonElem, &point.lon);

        // // Extract optional elements
        // tinyxml2::XMLElement* ele = trkpt->FirstChildElement("ele");
        // if (ele) point.ele = static_cast<float>(ele->DoubleText());

        // tinyxml2::XMLElement* time = trkpt->FirstChildElement("time");
        // if (time) point.time = strdup(time->GetText());

        trackData.push_back(point);
      }
    }
  }

    return true;
}

