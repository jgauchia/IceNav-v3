/**
 * @file gpxFiles.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Create GPX files and folder struct
 * @version 0.2.3
 * @date 2025-06
 */

#include "gpxFiles.hpp"
#include "esp_log.h"

wayPoint addWpt;  /**< Temporary waypoint structure used when adding a new waypoint. */
wayPoint loadWpt; /**< Temporary waypoint structure used when loading a waypoint. */
extern Storage storage;

static const char* TAG PROGMEM = "GPX file struct";

/**
 * @brief Create GPX folders structure
 *
 * @details Checks for the existence of the TRK and WPT folders on storage. Creates them if they do not exist,
 * 			and logs the results.
 */
void createGpxFolders()
{
	if (!storage.exists(trkFolder))
	{
		ESP_LOGI(TAG,"TRK folder not exists");
		if (storage.mkdir(trkFolder))
		ESP_LOGI(TAG, "TRK folder created");
		else
		ESP_LOGE(TAG, "TRK folder not created");
	}
	else
		ESP_LOGI(TAG,"TRK folder exists");

	if (!storage.exists(wptFolder))
	{
		ESP_LOGI(TAG,"WPT folder not exists");
		if (storage.mkdir(wptFolder))
		ESP_LOGI(TAG, "WPT folder created");
		else
		ESP_LOGE(TAG, "WPT folder not created");
	}
	else
		ESP_LOGI(TAG,"WPT folder exists");
}

/**
 * @brief Create default IceNav waypoint file
 *
 * @details Checks for the existence of the default waypoint GPX file on storage. If it does not exist,
 * 			the function creates a new GPX file with the GPX header. Logs the results.
 */
void createWptFile()
{
	FILE *gpxFile = storage.open(wptFile, "r");

	if (!gpxFile)
	{
		ESP_LOGI(TAG,"WPT file not exists");
		gpxFile = storage.open(wptFile, "w");
		if (gpxFile)
		{
		ESP_LOGI(TAG,"Creating WPT file");
		storage.println(gpxFile, gpxHeader);
		storage.close(gpxFile);
		ESP_LOGI(TAG,"file Size: %d", storage.size(wptFile));
		}
		else
		ESP_LOGE(TAG,"WPT file creation error");
	}
	else
	{
		ESP_LOGI(TAG,"WPT file exists");
		storage.close(gpxFile);
	}
}