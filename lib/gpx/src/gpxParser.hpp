/**
 * @file gpxParser.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPX Parser class
 * @version 0.3.0
 * @date 2026-06
 */

#pragma once

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
#include "gpsMath.hpp"

inline constexpr const char* TAGGPX = "GPXParser";

inline constexpr const char* gpxWaypointTag = "wpt";   /**< GPX waypoint tag. */
inline constexpr const char* gpxTrackTag    = "trk";   /**< GPX track tag. */
inline constexpr const char* gpxNameElem    = "name";  /**< GPX name element. */
inline constexpr const char* gpxLatElem     = "lat";   /**< GPX latitude attribute. */
inline constexpr const char* gpxLonElem     = "lon";   /**< GPX longitude attribute. */
inline constexpr const char* gpxEleElem     = "ele";   /**< GPX elevation element. */
inline constexpr const char* gpxTimeElem    = "time";  /**< GPX time element. */
inline constexpr const char* gpxDescElem    = "desc";  /**< GPX description element. */
inline constexpr const char* gpxSrcElem     = "src";   /**< GPX source element. */
inline constexpr const char* gpxSymElem     = "sym";   /**< GPX symbol element. */
inline constexpr const char* gpxTypeElem    = "type";  /**< GPX type element. */
inline constexpr const char* gpxSatElem     = "sat";   /**< GPX satellites element. */
inline constexpr const char* gpxHdopElem    = "hdop";  /**< GPX horizontal dilution of precision element. */
inline constexpr const char* gpxVdopElem    = "vdop";  /**< GPX vertical dilution of precision element. */
inline constexpr const char* gpxPdopElem    = "pdop";  /**< GPX position dilution of precision element. */


/**
 * @class GPXParser
 * @brief GPX file parser and editor class.
 */
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
        bool loadTrack(TrackVector& trackData);
        std::vector<TurnPoint> getTurnPointsSlidingWindow(float thresholdDeg, float minDist, float sharpTurnDeg,int windowSize, const TrackVector& trackData);

        std::string filePath;
};

/**
* @brief Edit a tag, attribute, or element in the GPX file.
*
* @param tag XML tag name.
* @param attribute XML attribute name (can be nullptr).
* @param element XML element name (can be nullptr).
* @param oldValue Old value to find.
* @param newValue New value to replace with.
* @return true if the edit was successful, false otherwise.
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
* @brief Insert a tag, attribute, or element in the GPX file.
* 
* @param tag XML tag name.
* @param attribute XML attribute name (can be nullptr).
* @param element XML element name (can be nullptr).
* @param value Value to insert.
* @return true if the insert was successful, false otherwise.
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