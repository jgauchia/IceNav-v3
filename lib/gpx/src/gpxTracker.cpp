#include "gpxTracker.hpp"
#include "esp_log.h"

extern Storage storage;
extern Gps gps;

static const char* TAG PROGMEM = "GPX Tracker";

std::string trackFile PROGMEM = ""; /**< Path to the waypoint GPX file on the SD card. */

std::string createFileName()
{
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    char buffer[64];
    strftime(buffer, sizeof(buffer), "/sdcard/TRK/track_%Y%m%d_%H%M%S.gpx", &timeinfo);

    std::string currentFileName = buffer;
    trackFile = currentFileName.c_str();
    return currentFileName;
}

void startTrack(){
    if (isTracking) {
        ESP_LOGW(TAG, "Tracking already started");
        return;
    } else {
        std::string fileName = createFileName();
        FILE* gpxFile = storage.open(fileName.c_str(), "w");
        if (!gpxFile) {
            ESP_LOGE(TAG, "Failed to create GPX file: %s", fileName.c_str());
            return;
        }

        storage.print(gpxFile, gpxHeader);
        storage.close(gpxFile);
        isTracking = true;
        ESP_LOGI(TAG, "Started tracking to file: %s", fileName.c_str());
    }
}

bool createTrackPoint(const trkPoint& tp) {
    tinyxml2::XMLDocument doc;
	tinyxml2::XMLError result = doc.LoadFile(trackFile.c_str());
	if (result != tinyxml2::XML_SUCCESS) 
	{
		ESP_LOGE(TAGGPX, "Failed to load file: %s", trackFile.c_str());
		return false;
	}

	tinyxml2::XMLElement* root = doc.RootElement();
	if (!root)  
	{
		ESP_LOGE(TAGGPX, "Failed to get root element in file: %s", trackFile.c_str());
		return false;
	}

	tinyxml2::XMLElement* trkElem = root->FirstChildElement(gpxTrackTag);
	if (!trkElem)
	{
		ESP_LOGE(TAGGPX, "Failed to get trk element in file: %s", trackFile.c_str());
		return false;
	}
	
	tinyxml2::XMLElement* trksegElem = trkElem->FirstChildElement(gpxTrackSegmentTag);
	if (!trksegElem)	
	{
		ESP_LOGE(TAGGPX, "Failed to get trkseg element in file: %s", trackFile.c_str());
		return false;
	}

	tinyxml2::XMLElement* newTrkPt = doc.NewElement(gpxTrackPointTag);
	newTrkPt->SetAttribute(gpxLatElem, formatFloat(tp.lat, 6).c_str());
	newTrkPt->SetAttribute(gpxLonElem, formatFloat(tp.lon, 6).c_str());

	tinyxml2::XMLElement* element = nullptr;

	element = doc.NewElement(gpxEleElem);
	element->SetText(tp.ele);
	newTrkPt->InsertEndChild(element);

	tinyxml2::XMLElement* lastTrkPt = trksegElem->LastChildElement(gpxTrackPointTag);
	if (lastTrkPt) 
		trksegElem->InsertAfterChild(lastTrkPt, newTrkPt);
	else
		trksegElem->InsertFirstChild(newTrkPt);

	result = doc.SaveFile(trackFile.c_str());
	if (result != tinyxml2::XML_SUCCESS)
	{
		ESP_LOGE(TAGGPX, "Failed to save file: %s", trackFile.c_str());
		return false;
	}
	return result == tinyxml2::XML_SUCCESS;
}

void stopTrack(){
	if (!isTracking) {
		ESP_LOGI(TAG, "Tracking not started");
		return;
	} else {
		isTracking = false;
		trackFile = "";
		ESP_LOGI(TAG, "Stopped tracking");
	}
}

std::string formatFloat(float value, int precision) 
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(precision) << value;
	return out.str();
}