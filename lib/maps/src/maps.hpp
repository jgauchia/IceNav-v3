/**
 * @file maps.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) - Render Maps
 * @brief  Maps draw class
 * @version 0.2.3
 * @date 2025-06
 */

#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include "tft.hpp"
#include "gpsMath.hpp"
#include "settings.hpp"
#include "compass.hpp"
#include "mapVars.h"
#include "storage.hpp"

/**
 * @class Maps
 * @brief Maps class for handling vector and raster map rendering, navigation, and viewport management.
 *
 */
class Maps
{
private:  
	/**
	* @brief Tile Map structure
	*
	*/
	struct MapTile
	{
		char file[255];      /**< File path of the map tile */
		uint32_t tilex;      /**< X index of the tile */
		uint32_t tiley;      /**< Y index of the tile */
		uint8_t zoom;        /**< Zoom level of the tile */
		float lat;          /**< Latitude of the tile center */
		float lon;          /**< Longitude of the tile center */
	};
	uint16_t lon2posx(float f_lon, uint8_t zoom, uint16_t tileSize);
	uint16_t lat2posy(float f_lat, uint8_t zoom, uint16_t tileSize);
	uint32_t lon2tilex(float f_lon, uint8_t zoom);
	uint32_t lat2tiley(float f_lat, uint8_t zoom);
	float tilex2lon(uint32_t tileX, uint8_t zoom);
	float tiley2lat(uint32_t tileY, uint8_t zoom);

	/**
	* @brief Map boundaries structure
	*
	* @details Structure that stores the geographic limits (minimum and maximum latitude and longitude)
	* 		   of a map tile.
	*/
	struct tileBounds
	{
		float lat_min;	   /**< Minimum latitude */
		float lat_max;     /**< Maximum latitude */
		float lon_min;     /**< Minimum longitude */
		float lon_max;     /**< Maximum longitude */
	};
	tileBounds totalBounds; /**< Map boundaries */
	/**
	* @brief Screen position from GPS coordinates
	*
	* @details Structure that stores screen coordinates (X, Y) derived from GPS data.
	*/
	struct ScreenCoord
	{
		uint16_t posX; /**< X position on screen */
		uint16_t posY; /**< Y position on screen */
	};
	static const uint16_t tileHeight = 768;                                      /**< Tile 9x9 Height Size */
	static const uint16_t tileWidth = 768;                                       /**< Tile 9x9 Width Size */
	static const uint16_t mapTileSize = 256;                             	     /**< Map tile size */
	static const uint16_t scrollThreshold = mapTileSize / 2;                     /**< Smooth scroll threshold */
	static const uint16_t vectorMapTileSize = tileHeight / 2;                    /**< Vector map tile size */
	uint16_t wptPosX, wptPosY;                                                   /**< Waypoint position on screen map */
	TFT_eSprite mapTempSprite = TFT_eSprite(&tft);                               /**< Full map sprite (not showed) */
	TFT_eSprite mapSprite = TFT_eSprite(&tft);                                   /**< Screen map sprite (showed) */
	float prevLat, prevLon;                                                      /**< Previous Latitude and Longitude */
	float destLat, destLon;                                                      /**< Waypoint destination latitude and longitude */
	uint8_t zoomLevel;                                                           /**< Zoom level for map display */
	ScreenCoord navArrowPosition; /**< Navigation Arrow position on screen */
    tileBounds getTileBounds(uint32_t tileX, uint32_t tileY, uint8_t zoom);
    bool isCoordInBounds(float lat, float lon, tileBounds bound);
    ScreenCoord coord2ScreenPos(float lon, float lat, uint8_t zoomLevel, uint16_t tileSize);
    void coords2map(float lat, float lon, tileBounds bound, uint16_t *pixelX, uint16_t *pixelY);
    void showNoMap(TFT_eSprite &map);

public:
	void* mapBuffer;                                               /**< Pointer to map screen sprite */
	uint16_t mapScrHeight;                                         /**< Screen map size height */
	uint16_t mapScrWidth;                                          /**< Screen map size width */
	uint16_t mapScrFull;                                           /**< Screen map size in full screen */
	bool redrawMap = true;                                         /**< Flag to indicate need redraw Map */
	bool followGps = true;                                         /**< Flag to indicate if map follow GPS signal */
	bool isMapFound = false;                                       /**< Flag to indicate when map is found on SD */
	MapTile oldMapTile;                                            /**< Old Map tile coordinates and zoom */
	MapTile currentMapTile;                                        /**< Current Map tile coordinates and zoom */
	MapTile roundMapTile;                                          /**< Boundaries Map tiles */
	int8_t tileX = 0;                                              /**< Map tile x counter */
	int8_t tileY = 0;                                              /**< Map tile y counter */
	int16_t offsetX = 0;                                           /**< Accumulative X scroll map offset */
	int16_t offsetY = 0;                                           /**< Accumulative Y scroll map offset */
	bool scrollUpdated = false;                                    /**< Flag to indicate when map was scrolled and needs to update */
	int8_t lastTileX = 0;                                          /**< Last Map tile x counter */
	int8_t lastTileY = 0;                                          /**< Last Map tile y counter */


    Maps();
    MapTile getMapTile(float lon, float lat, uint8_t zoomLevel, int8_t offsetX, int8_t offsetY);
    void initMap(uint16_t mapHeight, uint16_t mapWidth);
    void deleteMapScrSprites();
    void createMapScrSprites();
    void generateMap(uint8_t zoom);
    void displayMap();
    void setWaypoint(float wptLat, float wptLon);
    void updateMap();
    void panMap(int8_t dx, int8_t dy);
    void centerOnGps(float lat, float lon);
    void scrollMap(int16_t dx, int16_t dy);
    void preloadTiles(int8_t dirX, int8_t dirY);
	uint16_t getColorForType(uint8_t type_id);
	bool drawTileFile(const char* path, int16_t xOffset, int16_t yOffset, TFT_eSprite &map) ;
};