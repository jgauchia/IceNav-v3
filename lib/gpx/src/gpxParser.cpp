/**
 * @file gpxParser.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  GPX Parser class
 * @version 0.2.3
 * @date 2025-06
 */

#include "gpxParser.hpp"

/**
 * @brief Helper function to format double values to a specific number of decimal places
 *
 * @details Formats a double value as a string with the specified number of decimal places.
 *
 * @param value     Value in double format.
 * @param precision Number of decimal places to use.
 * @return std::string Formatted value as a string.
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
* @details Loads all track points from the `<trk>` structure in the GPX file, extracting latitude and longitude for each `<trkpt>`,
* 		   and stores them as `wayPoint` structures in the provided vector. Returns true if the operation was successful, or false on error.
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

/**
 * @brief Get turn points of a given track, filtering by minimum angle and distance, 
 *        but always detect sharp turns regardless of distance.
 *
 * This function analyzes the provided track data to find significant turn points.
 * A turn point is detected if the angle difference between two consecutive legs
 * exceeds thresholdDeg and the distance between points is greater than minDist,
 * or if the angle is very sharp (> sharpTurnDeg) regardless of distance.
 *
 * @param thresholdDeg Minimum angle difference (in degrees) to consider a turn.
 * @param minDist Minimum distance (in meters) between consecutive points to consider a turn.
 * @param sharpTurnDeg Angle (in degrees) above which any turn is considered important, even if distance is small. 
 * @param trackData Vector containing wayPoint structures representing the track.
 * @return std::vector<TurnPoint> Vector with detected turn points:
 *         - index: index in trackData of the turn
 *         - angle: angle difference at turn (deg, positive=right, negative=left)
 *         - accumDist: accumulated distance up to the turn (meters)
 */
std::vector<TurnPoint> GPXParser::getTurnPoints(float thresholdDeg, double minDist, float sharpTurnDeg, std::vector<wayPoint>& trackData)
{
  std::vector<TurnPoint> turnPoints;
  double accumDist = 0;
  if (trackData.size() < 3) 
    return turnPoints;
  
  for (size_t i = 1; i < trackData.size() - 1; ++i)
  {
    double distPrev = calcDist(trackData[i-1].lat, trackData[i-1].lon, trackData[i].lat, trackData[i].lon);
    accumDist += distPrev;

    double brg1 = calcCourse(trackData[i-1].lat, trackData[i-1].lon, trackData[i].lat, trackData[i].lon);
    double brg2 = calcCourse(trackData[i].lat, trackData[i].lon, trackData[i+1].lat, trackData[i+1].lon);
    double diff = calcAngleDiff(brg2, brg1);

    // Always consider very sharp turns, even if points are close
    if (fabs(diff) > sharpTurnDeg) 
    {
      turnPoints.push_back({static_cast<int>(i), diff, accumDist});
      continue;
    }

    // For less sharp turns, apply distance filter
    if (distPrev < minDist)
      continue;

    if (fabs(diff) > thresholdDeg)
      turnPoints.push_back({static_cast<int>(i), diff, accumDist});
  }
  
  return turnPoints;
}

/**
 * @brief Detect turn points in a track using a sliding window approach.
 *
 * This function processes the given track using a sliding window of configurable size.
 * It calculates the angle between the start and end segments of the window to detect turns.
 * A turn point is added if the global angle exceeds `thresholdDeg` and the total distance 
 * within the window is above `minDist`. Sharp turns above `sharpTurnDeg` are always included, 
 * regardless of distance.
 *
 * @param thresholdDeg Minimum angle difference (in degrees) to consider a turn.
 * @param minDist Minimum total distance (in meters) within the window to validate a turn.
 * @param sharpTurnDeg Angle (in degrees) above which any turn is considered sharp and relevant.
 * @param windowSize Number of points before and after the center point to define the sliding window.
 * @param trackData Vector of wayPoint structures representing the track to analyze.
 * @return std::vector<TurnPoint> List of detected turn points:
 *         - index: index in trackData of the turn
 *         - angle: angle difference at turn (degrees, positive = right, negative = left)
 *         - accumDist: accumulated distance from the start of the track to this turn (meters)
 */
std::vector<TurnPoint> GPXParser::getTurnPointsSlidingWindow(
    float thresholdDeg, double minDist, float sharpTurnDeg,
    int windowSize, const std::vector<wayPoint>& trackData)
{
    std::vector<TurnPoint> turnPoints;
    double accumDist = 0;
    if (trackData.size() < 2 * windowSize + 1)
        return turnPoints;

    for (size_t i = windowSize; i < trackData.size() - windowSize; ++i)
    {
        // Distancia total de la ventana (puedes usarla como filtro si quieres)
        double distWindow = 0;
        for (int j = int(i - windowSize); j < int(i + windowSize); ++j)
            distWindow += calcDist(trackData[j].lat, trackData[j].lon, trackData[j+1].lat, trackData[j+1].lon);

        // Ángulo global entre el tramo inicial y final de la ventana
        double brgStart = calcCourse(trackData[i - windowSize].lat, trackData[i - windowSize].lon,
                                     trackData[i].lat, trackData[i].lon);
        double brgEnd   = calcCourse(trackData[i].lat, trackData[i].lon,
                                     trackData[i + windowSize].lat, trackData[i + windowSize].lon);
        double diff = calcAngleDiff(brgEnd, brgStart);

        accumDist += calcDist(trackData[i-1].lat, trackData[i-1].lon, trackData[i].lat, trackData[i].lon);

        // Detección igual que el método clásico, pero sobre el ángulo "global" de la ventana
        if (std::abs(diff) > sharpTurnDeg) {
            turnPoints.push_back({static_cast<int>(i), diff, accumDist});
            continue;
        }
        if (distWindow < minDist)
            continue;
        if (std::abs(diff) > thresholdDeg)
            turnPoints.push_back({static_cast<int>(i), diff, accumDist});
    }
    return turnPoints;
}