/**
 * @file gpxFiles.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Create GPX files and folder struct
 * @version 0.2.9
 * @date 2026-06
 */

#include "gpxFiles.hpp"
#include "esp_log.h"

wayPoint addWpt;  /**< Temporary waypoint structure used when adding a new waypoint. */
wayPoint loadWpt; /**< Temporary waypoint structure used when loading a waypoint. */
extern Storage storage;

static const char* TAG = "GPX file struct";

/**
 * @brief Create a folder on storage if it does not already exist.
 *
 * @details Checks for the existence of the specified folder and creates it if missing.
 *          Logs the result of the operation.
 *
 * @param folder Absolute path of the folder to create.
 */
static void createFolderIfNotExists(const char* folder)
{
    if (!storage.exists(folder))
    {
        ESP_LOGI(TAG, "%s folder not exists", folder);
        if (storage.mkdir(folder))
            ESP_LOGI(TAG, "%s folder created", folder);
        else
            ESP_LOGE(TAG, "%s folder not created", folder);
    }
    else
        ESP_LOGI(TAG, "%s folder exists", folder);
}

/**
 * @brief Create GPX folders structure
 *
 * @details Checks for the existence of the TRK and WPT folders on storage. Creates them if they do not exist,
 * 			and logs the results.
 */
void createGpxFolders()
{
    createFolderIfNotExists(trkFolder);
    createFolderIfNotExists(wptFolder);
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