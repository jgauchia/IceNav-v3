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
#include "gpsMath.hpp"

extern std::vector<TrackSegment> trackIndex;

/**
 * @brief Helper function to format float values
 *
 * @param value Value to format.
 * @param precision Decimal places.
 * @return Formatted string.
 */
std::string formatFloat(float value, int precision)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}

/**
 * @brief Constructs a GPXParser
 *
 * @param filePath Path to the GPX file.
 */
GPXParser::GPXParser(const char* filePath) : filePath(filePath) {}
GPXParser::GPXParser() : filePath("") {}
GPXParser::~GPXParser() {}

/**
 * @brief Retrieve tag elements value list from all GPX files in a folder.
 *
 * @param tag XML tag (e.g., "wpt").
 * @param element XML element (e.g., "name").
 * @param folderPath Folder path.
 * @return Map of filename to vector of values.
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
                std::vector<std::string> elementValue;

                FILE* file = fopen(filePath.c_str(), "r");
                if (file)
                {
                    char line[256];
                    std::string startTag = "<" + std::string(tag);
                    std::string searchElement = "<" + std::string(element) + ">";
                    std::string endElement = "</" + std::string(element) + ">";
                    bool inTargetTag = false;

                    while (fgets(line, sizeof(line), file))
                    {
                        if (strstr(line, startTag.c_str()))
                            inTargetTag = true;

                        if (inTargetTag)
                        {
                            char* s = strstr(line, searchElement.c_str());
                            if (s)
                            {
                                s += searchElement.length();
                                char* e = strstr(s, endElement.c_str());
                                if (e)
                                {
                                    elementValue.push_back(std::string(s, e - s));
                                    inTargetTag = false; // Reset for next tag occurrence
                                }
                            }
                        }
                    }
                    fclose(file);
                }
                elementsByFile[fileName] = elementValue;
            }
        }
    }
    closedir(dir);
    return elementsByFile;
}

/**
 * @brief Delete a tag from the GPX file by name.
 *
 * @param tag XML tag.
 * @param name Name to match.
 * @return true if successful.
 */
bool GPXParser::deleteTagByName(const char* tag, const char* name)
{
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filePath.c_str()) != tinyxml2::XML_SUCCESS)
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
            if (doc.SaveFile(filePath.c_str()) != tinyxml2::XML_SUCCESS)
            {
                ESP_LOGE(TAGGPX, "Failed to save file: %s", filePath.c_str());
                return false;
            }
            return true;
        }
    }
    return false;
}

/**
 * @brief Retrieve waypoint details for a given name.
 *
 * @param name Waypoint name.
 * @return wayPoint structure.
 */
wayPoint GPXParser::getWaypointInfo(const char* name)
{
    wayPoint wp = {0};
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filePath.c_str()) != tinyxml2::XML_SUCCESS) return wp;
    tinyxml2::XMLElement* root = doc.RootElement();
    if (!root) return wp;
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
    return wp;
}

/**
 * @brief Add a new waypoint to the GPX file
 *
 * @param wp Waypoint structure.
 * @return true if successful.
 */
bool GPXParser::addWaypoint(const wayPoint& wp)
{
    time_t tUTCwpt = time(NULL);
    struct tm UTCwpt_tm;
    struct tm *tmUTCwpt = gmtime_r(&tUTCwpt, &UTCwpt_tm);
    char textFmt[30];
    strftime(textFmt, sizeof(textFmt), "%Y-%m-%dT%H:%M:%SZ", tmUTCwpt);
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filePath.c_str()) != tinyxml2::XML_SUCCESS) return false;
    tinyxml2::XMLElement* root = doc.RootElement();
    if (!root) return false;
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
    return doc.SaveFile(filePath.c_str()) == tinyxml2::XML_SUCCESS;
}

/**
* @brief Load GPX track data using stream-based parsing.
*
* @param trackData Vector to store points.
* @return true if successful.
*/
bool GPXParser::loadTrack(TrackVector& trackData)
{
    FILE* file = fopen(filePath.c_str(), "r");
    if (!file) 
        return false;
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);
    size_t estimatedPoints = fileSize / 50;
    trackData.reserve(estimatedPoints);
    trackIndex.clear();
    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        if (strstr(line, "<trkpt"))
        {
            wayPoint point = {0};
            bool latFound = false, lonFound = false;
            auto parseAttrs = [&](char* str) {
                char* pLat = strstr(str, "lat=\"");
                if (!pLat)
                    pLat = strstr(str, "lat='");
                if (pLat)
                { 
                    point.lat = strtof(pLat + 5, nullptr); 
                    latFound = true; 
                }
                char* pLon = strstr(str, "lon=\"");
                if (!pLon) 
                    pLon = strstr(str, "lon='");
                if (pLon) 
                { 
                    point.lon = strtof(pLon + 5, nullptr); 
                    lonFound = true; 
                }
            };
            parseAttrs(line);
            while ((!latFound || !lonFound) && fgets(line, sizeof(line), file)) {
                if (strstr(line, ">"))
                    break;
                parseAttrs(line);
            }
            if (latFound && lonFound) 
                trackData.push_back(point);
        }
    }
    fclose(file);
    if (!trackData.empty())
    {
        float totalDist = 0;
        trackData[0].accumDist = 0;
        const int SEGMENT_SIZE = 100;
        TrackSegment currentSeg;
        currentSeg.startIdx = 0;
        currentSeg.minLat = 90.0f; currentSeg.maxLat = -90.0f;
        currentSeg.minLon = 180.0f; currentSeg.maxLon = -180.0f;
        for (size_t i = 0; i < trackData.size(); ++i)
        {
            if (i > 0)
            {
                float d = calcDist(trackData[i-1].lat, trackData[i-1].lon, trackData[i].lat, trackData[i].lon);
                totalDist += d;
                trackData[i].accumDist = totalDist;
            }
            if (trackData[i].lat < currentSeg.minLat) 
                currentSeg.minLat = trackData[i].lat;
            if (trackData[i].lat > currentSeg.maxLat) 
                currentSeg.maxLat = trackData[i].lat;
            if (trackData[i].lon < currentSeg.minLon) 
                currentSeg.minLon = trackData[i].lon;
            if (trackData[i].lon > currentSeg.maxLon) 
                currentSeg.maxLon = trackData[i].lon;
            if ((i + 1) % SEGMENT_SIZE == 0 || i == trackData.size() - 1)
            {
                currentSeg.endIdx = i;
                const float BUFFER = 0.0005f; 
                currentSeg.minLat -= BUFFER; currentSeg.maxLat += BUFFER;
                currentSeg.minLon -= BUFFER; currentSeg.maxLon += BUFFER;
                trackIndex.push_back(currentSeg);
                if (i < trackData.size() - 1)
                {
                    currentSeg.startIdx = i + 1;
                    currentSeg.minLat = 90.0f; currentSeg.maxLat = -90.0f;
                    currentSeg.minLon = 180.0f; currentSeg.maxLon = -180.0f;
                }
            }
        }
        ESP_LOGI(TAGGPX, "Index built. Segments: %d, Total Dist: %.1f m", trackIndex.size(), totalDist);
    }
    return true;
}

/**
 * @brief Detects turn points using sliding window
 *
 * @param thresholdDeg Min angle.
 * @param minDist Min distance.
 * @param sharpTurnDeg Sharp turn threshold.
 * @param windowSize Window size.
 * @param trackData Track data.
 * @return Vector of TurnPoints.
 */
std::vector<TurnPoint> GPXParser::getTurnPointsSlidingWindow(
    float thresholdDeg, float minDist, float sharpTurnDeg,
    int windowSize, const TrackVector& trackData)
{
    std::vector<TurnPoint> turnPoints;
    if (trackData.size() < 2 * windowSize + 1) return turnPoints;
    turnPoints.reserve(trackData.size() / 20);
    for (size_t i = windowSize; i < trackData.size() - windowSize; ++i)
    {
        float distWindow = 0.0f;
        bool skipWindow = false;
        for (int j = int(i - windowSize); j < int(i + windowSize); ++j)
        {
            float d = calcDist(trackData[j].lat, trackData[j].lon, trackData[j + 1].lat, trackData[j + 1].lon);
            if (d > 200.0f) 
            { 
                skipWindow = true; 
                break; 
            }
            distWindow += d;
        }
        if (skipWindow) 
            continue;
        float brgStart = calcCourse(trackData[i - windowSize].lat, trackData[i - windowSize].lon, trackData[i].lat, trackData[i].lon);
        float brgEnd   = calcCourse(trackData[i].lat, trackData[i].lon, trackData[i + windowSize].lat, trackData[i + windowSize].lon);
        float diff = calcAngleDiff(brgEnd, brgStart);
        if (std::fabs(diff) > sharpTurnDeg)
        {
            turnPoints.push_back({static_cast<int>(i), diff, trackData[i].accumDist});
            continue;
        }
        if (distWindow < minDist) 
            continue;
        if (std::fabs(diff) > thresholdDeg) 
            turnPoints.push_back({static_cast<int>(i), diff, trackData[i].accumDist});
    }
    return turnPoints;
}