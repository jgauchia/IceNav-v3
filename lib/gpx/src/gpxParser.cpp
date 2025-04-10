/**
 * @file gpxParser.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPX Parser class
 * @version 0.2.0
 * @date 2025-04
 */

#include "gpxParser.hpp"
#include <iomanip>
#include <sstream>
#include "esp_log.h"

static const char* TAG PROGMEM = "GPXParser";

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

GPXParser::~GPXParser() {}

/**
 * @brief Retrieve Waypoint list
 *
 * @param filePath GPX file path
 * @return std::vector<std::string> Vector containing all waypoint names
 */
 std::vector<std::string> GPXParser::getWaypointList() 
 {
    std::vector<std::string> names;

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.LoadFile(filePath.c_str());
    if (result != tinyxml2::XML_SUCCESS) 
    {
      ESP_LOGE(TAG, "Failed to load file: %s", filePath.c_str());
      return names;
    }

    tinyxml2::XMLElement* root = doc.RootElement();
    if (!root)
    {
      ESP_LOGE(TAG, "Failed to get root element in file: %s", filePath.c_str());
      return names;
    }

    for (tinyxml2::XMLElement* wpt = root->FirstChildElement("wpt"); wpt != nullptr; wpt = wpt->NextSiblingElement("wpt")) 
    {
      tinyxml2::XMLElement* nameElement = wpt->FirstChildElement("name");
      if (nameElement) 
      {
        const char* name = nameElement->GetText();
        if (name)
          names.push_back(name);
      }
    }

    std::sort(names.begin(), names.end());
    
    return names;
}

/**
 * @brief Retrieve waypoint details for a given name from a GPX file
 *
 * @param name Waypoint name to search for
 * @return wayPoint Struct containing waypoint details
 */
 wayPoint GPXParser::getWaypointInfo(const String& name) 
 {
    wayPoint wp = {0};

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.LoadFile(filePath.c_str());
    if (result != tinyxml2::XML_SUCCESS)
    {
      ESP_LOGE(TAG, "Failed to load file: %s", filePath.c_str());
      return wp;
    }

    tinyxml2::XMLElement* root = doc.RootElement();
    if (!root) 
    {
      ESP_LOGE(TAG, "Failed to get root element in file: %s", filePath.c_str());
      return wp;
    }

    tinyxml2::XMLElement* wpt = nullptr;
    for (wpt = root->FirstChildElement("wpt"); wpt != nullptr; wpt = wpt->NextSiblingElement("wpt")) 
    {
      tinyxml2::XMLElement* nameElement = wpt->FirstChildElement("name");
      if (nameElement && name == nameElement->GetText()) 
      {
        wp.name = strdup(nameElement->GetText());
        break;
      }
    }

    if (wpt)
    {
      wpt->QueryDoubleAttribute("lat", &wp.lat);
      wpt->QueryDoubleAttribute("lon", &wp.lon);

      tinyxml2::XMLElement* element = nullptr;

      element = wpt->FirstChildElement("ele");
      if (element) 
        wp.ele = static_cast<float>(element->DoubleText());

      element = wpt->FirstChildElement("time");
      if (element)
        wp.time = strdup(element->GetText());

      element = wpt->FirstChildElement("desc");
      if (element) 
        wp.desc = strdup(element->GetText());

      element = wpt->FirstChildElement("src");
      if (element) 
        wp.src = strdup(element->GetText());

      element = wpt->FirstChildElement("sym");
      if (element) 
        wp.sym = strdup(element->GetText());

      element = wpt->FirstChildElement("type");
      if (element) 
        wp.type = strdup(element->GetText());

      element = wpt->FirstChildElement("sat");
      if (element) 
        wp.sat = static_cast<uint8_t>(element->UnsignedText());

      element = wpt->FirstChildElement("hdop");
      if (element) 
        wp.hdop = static_cast<float>(element->DoubleText());

      element = wpt->FirstChildElement("vdop");
      if (element) 
        wp.vdop = static_cast<float>(element->DoubleText());

      element = wpt->FirstChildElement("pdop");
      if (element) 
        wp.pdop = static_cast<float>(element->DoubleText());
    }
    else
        ESP_LOGE(TAG, "Waypoint with name %s not found in file: %s", name.c_str(), filePath.c_str());

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
    ESP_LOGE(TAG, "Failed to load file: %s", filePath.c_str());
    return false;
  }

  tinyxml2::XMLElement* root = doc.RootElement();
  if (!root) 
{
    ESP_LOGE(TAG, "Failed to get root element in file: %s", filePath.c_str());
    return false;
  }

  tinyxml2::XMLElement* newWpt = doc.NewElement("wpt");
  newWpt->SetAttribute("lat", formatDouble(wp.lat, 6).c_str());
  newWpt->SetAttribute("lon", formatDouble(wp.lon, 6).c_str());

  tinyxml2::XMLElement* element = nullptr;

  element = doc.NewElement("ele");
  element->SetText(wp.ele);
  newWpt->InsertEndChild(element);

  element = doc.NewElement("time");
  element->SetText(textFmt);
  newWpt->InsertEndChild(element);

  element = doc.NewElement("name");
  element->SetText(wp.name ? wp.name : "");
  newWpt->InsertEndChild(element);

  // element = doc.NewElement("desc");
  // element->SetText(wp.desc ? wp.desc : "");
  // newWpt->InsertEndChild(element);

  element = doc.NewElement("src");
  element->SetText(wp.src ? wp.src : "IceNav");
  newWpt->InsertEndChild(element);

  // element = doc.NewElement("sym");
  // element->SetText(wp.sym ? wp.sym : "");
  // newWpt->InsertEndChild(element);

  // element = doc.NewElement("type");
  // element->SetText(wp.type ? wp.type : "");
  // newWpt->InsertEndChild(element);

  element = doc.NewElement("sat");
  element->SetText(wp.sat);
  newWpt->InsertEndChild(element);

  element = doc.NewElement("hdop");
  element->SetText(wp.hdop);
  newWpt->InsertEndChild(element);

  element = doc.NewElement("vdop");
  element->SetText(wp.vdop);
  newWpt->InsertEndChild(element);

  element = doc.NewElement("pdop");
  element->SetText(wp.pdop);
  newWpt->InsertEndChild(element);

  tinyxml2::XMLElement* lastWpt = root->LastChildElement("wpt");
  if (lastWpt) 
    root->InsertAfterChild(lastWpt, newWpt);
  else
    root->InsertFirstChild(newWpt);

  result = doc.SaveFile(filePath.c_str());
  if (result != tinyxml2::XML_SUCCESS)
  {
    ESP_LOGE(TAG, "Failed to save file: %s", filePath.c_str());
    return false;
  }
  return result == tinyxml2::XML_SUCCESS;
}

/**
 * @brief Edit the name of an existing waypoint in the GPX file
 *
 * @param oldName The old name of the waypoint
 * @param newName The new name of the waypoint
 * @return true if the waypoint name was edited successfully, false otherwise
 */
 bool GPXParser::editWaypointName(const char* oldName, const char* newName)
 {
  tinyxml2::XMLDocument doc;
  tinyxml2::XMLError result = doc.LoadFile(filePath.c_str());
  if (result != tinyxml2::XML_SUCCESS) 
  {
    ESP_LOGE(TAG, "Failed to load file: %s", filePath.c_str());
    return false;
  }

  tinyxml2::XMLElement* root = doc.RootElement();
  if (!root)
  {
    ESP_LOGE(TAG, "Failed to get root element in file: %s", filePath.c_str());
    return false;
  }

  for (tinyxml2::XMLElement* wpt = root->FirstChildElement("wpt"); wpt != nullptr; wpt = wpt->NextSiblingElement("wpt")) 
  {
    tinyxml2::XMLElement* nameElement = wpt->FirstChildElement("name");
    if (nameElement && strcmp(nameElement->GetText(), oldName) == 0)
    {
      nameElement->SetText(newName);
      result = doc.SaveFile(filePath.c_str());
      if (result != tinyxml2::XML_SUCCESS)
      {
        ESP_LOGE(TAG, "Failed to save file: %s", filePath.c_str());
        return false;
      }
      return true;
    }
  }

  ESP_LOGE(TAG, "Waypoint with name %s not found in file: %s", oldName, filePath.c_str());
  return false;
}

/**
 * @brief Delete a waypoint from the GPX file
 *
 * @param name The name of the waypoint to delete
 * @return true if the waypoint was deleted successfully, false otherwise
 */
 bool GPXParser::deleteWaypoint(const char* name)
 {
  tinyxml2::XMLDocument doc;
  tinyxml2::XMLError result = doc.LoadFile(filePath.c_str());
  if (result != tinyxml2::XML_SUCCESS) 
  {
    ESP_LOGE("GPXParser", "Failed to load file: %s", filePath.c_str());
    return false;
  }

  tinyxml2::XMLElement* root = doc.RootElement();
  if (!root) 
  {
    ESP_LOGE("GPXParser", "Failed to get root element in file: %s", filePath.c_str());
    return false;
  }

  for (tinyxml2::XMLElement* wpt = root->FirstChildElement("wpt"); wpt != nullptr; wpt = wpt->NextSiblingElement("wpt")) 
  {
    tinyxml2::XMLElement* nameElement = wpt->FirstChildElement("name");
    if (nameElement && strcmp(nameElement->GetText(), name) == 0) 
    {
      root->DeleteChild(wpt);
      result = doc.SaveFile(filePath.c_str());
      if (result != tinyxml2::XML_SUCCESS)
      {
        ESP_LOGE("GPXParser", "Failed to save file: %s", filePath.c_str());
        return false;
      }
      return true;
    }
  }

  ESP_LOGE("GPXParser", "Waypoint with name %s not found in file: %s", name, filePath.c_str());
  return false;
}