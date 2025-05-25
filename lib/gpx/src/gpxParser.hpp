/**
 * @file gpxParser.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPX Parser class
 * @version 0.2.1
 * @date 2025-05
 */

#pragma once

#include <Arduino.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <dirent.h> 
#include "esp_log.h"
#include "tinyxml2.h"
#include "globalGpxDef.h"

static const char* TAGGPX PROGMEM = "GPXParser";

static const char* gpxWaypointTag PROGMEM = "wpt";
static const char* gpxTrackTag PROGMEM = "trk";
static const char* gpxNameElem PROGMEM = "name";
static const char* gpxLatElem PROGMEM = "lat";
static const char* gpxLonElem PROGMEM = "lon";
static const char* gpxEleElem PROGMEM ="ele";
static const char* gpxTimeElem PROGMEM = "time";
static const char* gpxDescElem PROGMEM = "desc";
static const char* gpxSrcElem PROGMEM = "src";
static const char* gpxSymElem PROGMEM = "sym";
static const char* gpxTypeElem PROGMEM = "type";
static const char* gpxSatElem PROGMEM = "sat";
static const char* gpxHdopElem PROGMEM = "hdop";
static const char* gpxVdopElem PROGMEM = "vdop";
static const char* gpxPdopElem PROGMEM = "pdop";


class GPXParser
{
  public:
    GPXParser(const char* filePath);
    GPXParser();
    ~GPXParser();

    
    template <typename T>
    bool editTagAttrOrElem(const char* tag, const char* attribute, const char* element, const T& oldValue, const T& newValue);
    template <typename T>
    bool insertTagAttrOrElem(const char* tag, const char* attribute, const char* element, const T& value);

    static std::map<std::string, std::vector<std::string>> getTagElementList(const char* tag, const char* element, const std::string& folderPath);
    bool deleteTagByName(const char* tag, const char* name);
    wayPoint getWaypointInfo(const char* name);
    bool addWaypoint(const wayPoint& wp);
    bool loadTrack(std::vector<wayPoint>& trackData);

    std::string filePath;
};

/**
 * @brief Edit tag attribute or element value in the GPX file
 *
 * @param tag 
 * @param attribute nullptr for edit element
 * @param element nullptr for edit attribute
 * @param oldValue The old value of the attribute or element
 * @param newValue The new value of the attribute or element
 * @return true if the tag attribute or element value was edited successfully, false otherwise
 */
template <typename T>
bool GPXParser::editTagAttrOrElem(const char* tag, const char* attribute, const char* element, const T& oldValue, const T& newValue)
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
 
  std::ostringstream oldValueStream, newValueStream;
  oldValueStream << oldValue;
  newValueStream << newValue;
  std::string oldValueStr = oldValueStream.str();
  std::string newValueStr = newValueStream.str();
 
  for (tinyxml2::XMLElement* tagElement = root->FirstChildElement(tag); tagElement != nullptr; tagElement = tagElement->NextSiblingElement(tag))
  {
    if (attribute)
    {
      const char* attrValue = tagElement->Attribute(attribute);
      if (attrValue && oldValueStr == attrValue)
      {
        tagElement->SetAttribute(attribute, newValueStr.c_str()); 
        result = doc.SaveFile(filePath.c_str());
        if (result != tinyxml2::XML_SUCCESS)
        {
          ESP_LOGE(TAGGPX, "Failed to save file: %s", filePath.c_str());
          return false;
        }
        return true; 
      }
    }
    else if (element)
    {
      tinyxml2::XMLElement* childElement = tagElement->FirstChildElement(element);
      if (childElement && childElement->GetText() && oldValueStr == childElement->GetText())
      {
        childElement->SetText(newValueStr.c_str());
        result = doc.SaveFile(filePath.c_str());
        if (result != tinyxml2::XML_SUCCESS)
        {
          ESP_LOGE(TAGGPX, "Failed to save file: %s", filePath.c_str());
          return false;
        }
        return true; 
      }
    }
  }

  ESP_LOGE(TAGGPX, "Attribute/Element '%s' with value '%s' not found for tag '%s' in file: %s",
          attribute ? attribute : element, oldValueStr.c_str(), tag, filePath.c_str());
  return false;
}
 
/**
 * @brief Insert the tag attribute or element value in the GPX file
 *
 * @param tag
 * @param attribute nullptr for insert element
 * @param element nullptr for insert attribute
 * @param value value of the attribute or element
 * @return true if the tag attribute or element value was inserted successfully, false otherwise
 */
template <typename T>
bool GPXParser::insertTagAttrOrElem(const char* tag, const char* attribute, const char* element, const T& value)
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

  tinyxml2::XMLElement* tagElement = root->FirstChildElement(tag);
  if (!tagElement)
  {
      ESP_LOGE(TAGGPX, "Tag '%s' not found in file: %s", tag, filePath.c_str());
      return false;
  }

  std::ostringstream valueStream;
  valueStream << value;
  std::string valueStr = valueStream.str();

  if (attribute)
    tagElement->SetAttribute(attribute, valueStr.c_str());
  else if (element)
  {
    tinyxml2::XMLElement* childElement = tagElement->FirstChildElement(element);
    if (!childElement)
    {
      childElement = doc.NewElement(element);
      if (!childElement)
      {
        ESP_LOGE(TAGGPX, "Failed to create new element '%s'", element);
        return false;
      }
      tagElement->InsertEndChild(childElement);
    }
    childElement->SetText(valueStr.c_str());
  }

  result = doc.SaveFile(filePath.c_str());
  if (result != tinyxml2::XML_SUCCESS)
  {
    ESP_LOGE(TAGGPX, "Failed to save file after inserting attribute/element: %s", filePath.c_str());
    return false;
  }

  ESP_LOGI(TAGGPX, "Successfully inserted attribute/element '%s' into tag '%s'", attribute ? attribute : element, tag);
  return true; 
}