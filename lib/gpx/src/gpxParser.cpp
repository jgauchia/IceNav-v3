/**
 * @file gpxParser.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPX Parser class
 * @version 0.2.4
 * @date 2025-12
 */

#include "gpxParser.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/**
 * @brief Helper function to format float values to a specific number of decimal places
 *
 * @details Formats a float value as a string with the specified number of decimal places.
 *
 * @param value     Value in float format.
 * @param precision Number of decimal places to use.
 * @return std::string Formatted value as a string.
 */
std::string formatFloat(float value, int precision)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}

/**
 * @brief Constructs a GPXParser with a specific file path.
 * @param filePath Path to the GPX file to be parsed.
 */
GPXParser::GPXParser(const char* filePath) : filePath(filePath) {}

/**
 * @brief Constructs a GPXParser with an empty file path.
 */
GPXParser::GPXParser() : filePath("") {}

/**
 * @brief Destructor for GPXParser.
 */
GPXParser::~GPXParser() {}

/**
 * @brief Retrieve tag elements value list from all GPX files in a specified folder.
 *
 * @details Iterates over all .gpx files in the given folder, extracts the specified element values under the specified tag,
 * 			and returns a map where each key is a file name and each value is a vector of element values.
 *
 * @param tag XML tag to look for (e.g., "wpt" for waypoint).
 * @param element XML element to extract (e.g., "name", "desc").
 * @param folderPath Path to the folder containing GPX files.
 * @return std::map<std::string, std::vector<std::string>> Map where the key is the file name and the value is a vector of tag element values.
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
 * @details Loads the GPX file, searches for the first tag whose <name> element matches the provided name,
 * 			deletes it, and saves the file. Returns true if deletion was successful, false otherwise.
 *
 * @param tag  The XML tag to search for (e.g., "wpt" for waypoint).
 * @param name The name value to match for deletion.
 * @return true if the tag was deleted successfully, false otherwise.
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

             return true;
        }
    }

    ESP_LOGE(TAGGPX, "Tag with name %s not found in file: %s", name, filePath.c_str());
    return false;
}

/**
 * @brief Retrieve waypoint details for a given name from a GPX file
 *
 * @details Searches the GPX file for a waypoint with the given name and returns its details.
 *
 * @param name Waypoint name to search for
 * @return wayPoint Struct containing waypoint details (fields are zeroed/empty if not found)
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
        wpt->QueryFloatAttribute(gpxLatElem, &wp.lat);
        wpt->QueryFloatAttribute(gpxLonElem, &wp.lon);

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
 * @details Adds a new waypoint with the provided details to the GPX file. All required GPX elements are populated,
 * 			and the waypoint is inserted at the end of the waypoint list. Returns true if the waypoint was added successfully,
 *			false otherwise.
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
    newWpt->SetAttribute(gpxLatElem, formatFloat(wp.lat, 6).c_str());
    newWpt->SetAttribute(gpxLonElem, formatFloat(wp.lon, 6).c_str());

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
* @details Loads all track points from the `<trk>` structure in the GPX file, extracting latitude and longitude for each `<trkpt>`,
* 		   and stores them as `wayPoint` structures in the provided vector. Pre-reserves memory to avoid heap fragmentation.
*          Returns true if the operation was successful, or false on error.
*
* @param trackData Vector to store track points (each point is a wayPoint).
* @return true if the track data was loaded successfully, false otherwise.
*/
bool GPXParser::loadTrack(TrackVector& trackData)
{
    FILE* file = fopen(filePath.c_str(), "r");
    if (!file)
    {
        ESP_LOGE(TAGGPX, "Failed to load file: %s", filePath.c_str());
        return false;
    }

    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        // Simple and fast parsing for <trkpt ... lat="..." lon="...">
        if (strstr(line, "<trkpt"))
        {
            wayPoint point = {0};
            bool latFound = false;
            bool lonFound = false;

            // Search for lat/lon in current line
            // Helper lambda to parse attributes
            auto parseAttrs = [&](char* str) {
                char* pLat = strstr(str, "lat=\"");
                if (!pLat) pLat = strstr(str, "lat='");
                if (pLat) {
                    point.lat = strtof(pLat + 5, nullptr);
                    latFound = true;
                }

                char* pLon = strstr(str, "lon=\"");
                if (!pLon) pLon = strstr(str, "lon='");
                if (pLon) {
                    point.lon = strtof(pLon + 5, nullptr);
                    lonFound = true;
                }
            };

            parseAttrs(line);

            // If not found, read next lines until we find them or close tag
            while ((!latFound || !lonFound) && fgets(line, sizeof(line), file)) {
                if (strstr(line, ">")) break; // End of tag properties
                parseAttrs(line);
            }

            if (latFound && lonFound)
            {
                trackData.push_back(point);
            }
        }
    }

    fclose(file);
    ESP_LOGI(TAGGPX, "Track loaded. Points: %d", trackData.size());
    return true;
}

/**
 * @brief Detects turn points in a GPX track using a sliding window approach.
 *
 * @details This function analyzes a track using a window of N points before and after each point
 *          to estimate the turning angle between segments. It avoids false positives by skipping
 *          windows with abnormally large jumps (e.g., GPS noise or corrupt data).
 *
 * @param thresholdDeg   Minimum angle (deg) to consider a turn.
 * @param minDist        Minimum distance (m) of the whole window to consider a valid curve.
 * @param sharpTurnDeg   Angle threshold (deg) for forced sharp turns.
 * @param windowSize     Number of points before/after to use in the window.
 * @param trackData      The vector of waypoints forming the track.
 * @return               A vector of TurnPoint structures.
 */
std::vector<TurnPoint> GPXParser::getTurnPointsSlidingWindow(
    float thresholdDeg, float minDist, float sharpTurnDeg,
    int windowSize, const TrackVector& trackData)
{
    std::vector<TurnPoint> turnPoints;
    float accumDist = 0.0f;

    if (trackData.size() < 2 * windowSize + 1)
        return turnPoints;

    // Reserve estimated capacity to avoid reallocations
    turnPoints.reserve(trackData.size() / 20);  // Estimate ~5% of points are turns

    for (size_t i = windowSize; i < trackData.size() - windowSize; ++i)
    {
        float distWindow = 0.0f;
        bool skipWindow = false;

        for (int j = int(i - windowSize); j < int(i + windowSize); ++j)
        {
            float d = calcDist(trackData[j].lat, trackData[j].lon,
                               trackData[j + 1].lat, trackData[j + 1].lon);

            if (d > 200.0f)
            { // Skip if suspicious jump
                skipWindow = true;
                break;
            }

            distWindow += d;
        }

        if (skipWindow)
            continue;

        float brgStart = calcCourse(trackData[i - windowSize].lat, trackData[i - windowSize].lon,
                                    trackData[i].lat, trackData[i].lon);
        float brgEnd   = calcCourse(trackData[i].lat, trackData[i].lon,
                                    trackData[i + windowSize].lat, trackData[i + windowSize].lon);
        float diff = calcAngleDiff(brgEnd, brgStart);

        accumDist += calcDist(trackData[i - 1].lat, trackData[i - 1].lon,
                              trackData[i].lat, trackData[i].lon);

        if (std::fabs(diff) > sharpTurnDeg)
        {
            turnPoints.push_back({static_cast<int>(i), diff, accumDist});
            continue;
        }

        if (distWindow < minDist)
            continue;

        if (std::fabs(diff) > thresholdDeg)
            turnPoints.push_back({static_cast<int>(i), diff, accumDist});
    }

    return turnPoints;
}
