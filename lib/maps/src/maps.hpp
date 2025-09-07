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
#include "tft.hpp"
#include "gpsMath.hpp"
#include "settings.hpp"
#include "compass.hpp"
#include "mapVars.h"
#include "storage.hpp"

/**
 * @brief Draw command types used for rendering vector graphics.
 * 
 * @details These commands are synchronized with the corresponding Python script
 * 			that generates the binary tile data. Each command corresponds to a specific
 * 			geometric drawing operation.
 */
enum DrawCommand : uint8_t 
{
    DRAW_LINE = 1,               /**< Draw a simple line segment */
    DRAW_POLYLINE = 2,           /**< Draw a series of connected line segments */
    DRAW_STROKE_POLYGON = 3,     /**< Draw only the stroke (outline) of a single polygon */
    DRAW_STROKE_POLYGONS = 4,    /**< Draw only the strokes (outlines) of multiple polygons */
    DRAW_HORIZONTAL_LINE = 5,    /**< Draw a horizontal line */
    DRAW_VERTICAL_LINE = 6,      /**< Draw a vertical line */
    SET_COLOR = 0x80,            /**< Set current drawing color (RGB332) */
    SET_COLOR_INDEX = 0x81,      /**< Set current color using palette index */
    RECTANGLE = 0x82,            /**< Optimized rectangle command */
    STRAIGHT_LINE = 0x83,        /**< Optimized straight line command */
    CIRCLE = 0x87,               /**< Optimized circle command */
                  };

/**
 * @class Maps
 * @brief Maps class for handling vector and raster map rendering, navigation, and viewport management.
 *
 */
class Maps
{
private:  
	struct MapTile 			/**< Tile Map structure */
	{
		char file[255];		/**< File path of the map tile */
		uint32_t tilex;     /**< X index of the tile */
		uint32_t tiley;     /**< Y index of the tile */
		uint8_t zoom;       /**< Zoom level of the tile */
		float lat;          /**< Latitude of the tile center */
		float lon;          /**< Longitude of the tile center */
	};
	struct tileBounds		/**< Map boundaries structure */
	{
		float lat_min;	   /**< Minimum latitude */
		float lat_max;     /**< Maximum latitude */
		float lon_min;     /**< Minimum longitude */
		float lon_max;     /**< Maximum longitude */
	};
	struct ScreenCoord		/**< Screen position from GPS coordinates */
	{
		uint16_t posX;		/**< X position on screen */
		uint16_t posY; 		/**< Y position on screen */
	};

	static constexpr int TILE_SIZE = 255;
	static constexpr int TILE_SIZE_PLUS_ONE = 256;
	static constexpr int MARGIN_PIXELS = 1;
	static const uint16_t tileHeight = 768;                                      /**< Tile 9x9 Height Size */
	static const uint16_t tileWidth = 768;                                       /**< Tile 9x9 Width Size */
	static const uint16_t mapTileSize = 256;                             	     /**< Map tile size */
	static const uint16_t scrollThreshold = mapTileSize / 2;                     /**< Smooth scroll threshold */

	static uint16_t currentDrawColor;                           				/**< Current drawing color state */
	static uint8_t PALETTE[256];                                                /**< Color palette for vector tiles */
	static uint32_t PALETTE_SIZE;                                               /**< Number of entries in the palette */
	
	tileBounds totalBounds; 													/**< Map boundaries */
	uint16_t wptPosX, wptPosY;                                                  /**< Waypoint position on screen map */
	TFT_eSprite mapTempSprite = TFT_eSprite(&tft);                              /**< Full map sprite (not showed) */
	TFT_eSprite mapSprite = TFT_eSprite(&tft);                                  /**< Screen map sprite (showed) */
	float destLat, destLon;                                                     /**< Waypoint destination latitude and longitude */
	uint8_t zoomLevel;                                                          /**< Zoom level for map display */
	ScreenCoord navArrowPosition; 												/**< Navigation Arrow position on screen */

	static uint16_t lon2posx(float f_lon, uint8_t zoom, uint16_t tileSize);
	static uint16_t lat2posy(float f_lat, uint8_t zoom, uint16_t tileSize);
	static uint32_t lon2tilex(float f_lon, uint8_t zoom);
	static uint32_t lat2tiley(float f_lat, uint8_t zoom);
	static float tilex2lon(uint32_t tileX, uint8_t zoom);
	static float tiley2lat(uint32_t tileY, uint8_t zoom);

    tileBounds getTileBounds(uint32_t tileX, uint32_t tileY, uint8_t zoom);
    bool isCoordInBounds(float lat, float lon, tileBounds bound);
    ScreenCoord coord2ScreenPos(float lon, float lat, uint8_t zoomLevel, uint16_t tileSize);
    void coords2map(float lat, float lon, tileBounds bound, uint16_t *pixelX, uint16_t *pixelY);
    void showNoMap(TFT_eSprite &map);
	void panMap(int8_t dx, int8_t dy);

	bool loadPalette(const char* palettePath);											/**< Load color palette from binary file */
	uint8_t paletteToRGB332(const uint32_t idx);					 					/**< Convert palette index to RGB332 color */
	uint8_t darkenRGB332(const uint8_t color, const float amount);  					/**< Darken RGB332 color by a given fraction */
	uint16_t RGB332ToRGB565(const uint8_t color);										/**< Convert RGB332 color to RGB565 */
	uint32_t readVarint(const uint8_t* data, size_t& offset, size_t dataSize);			/**< Read a varint-encoded uint32_t from data buffer */
	int32_t readZigzag(const uint8_t* data, size_t& offset, const size_t dataSize);		/**< Read a zigzag-encoded int32_t from data buffer */
	int uint16ToPixel(const int32_t val);												/**< Convert uint16_t tile coordinate to pixel coordinate */
	bool isPointOnMargin(const int px, const int py);									/**< Check if a point is on the margin of the tile */
	bool isNear(int val, int target, int tol);											/**< Check if a value is near a target within a tolerance */
	bool shouldDrawLine(const int px1, const int py1, const int px2, const int py2);	/**< Determine if a line should be drawn based on its endpoints */
	void fillPolygonGeneral(TFT_eSprite &map, const int *px, const int *py, 
							const int numPoints, const uint16_t color,	
							const int xOffset, const int yOffset);						/**< Fill a polygon using the scanline algorithm */
	void drawPolygonBorder(TFT_eSprite &map, const int *px, const int *py, 
							const int numPoints, const uint16_t borderColor, const uint16_t fillColor, 
							const int xOffset, const int yOffset);						/**< Draw the border of a polygon with margin handling */
	template<typename T>
    T* allocBuffer(size_t numElements);													/**< Allocate a buffer with memory caps */			
	void drawFilledRectWithBorder(TFT_eSprite& map, int x, int y, int w, int h, 
		                          uint16_t fill, uint16_t border, bool fillShape);		/**< Draw filled rectangle with border */
	void drawFilledCircleWithBorder(TFT_eSprite& map, int x, int y, int r,
									uint16_t fill, uint16_t border, bool fillShape);	/**< Draw filled circle with border */											

   
public:
	bool fillPolygons;                                             /**< Flag for polygon filling */
	void* mapBuffer;                                               /**< Pointer to map screen sprite */
	uint16_t mapScrHeight;                                         /**< Screen map size height */
	uint16_t mapScrWidth;                                          /**< Screen map size width */
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
    void centerOnGps(float lat, float lon);
    void scrollMap(int16_t dx, int16_t dy);
    void preloadTiles(int8_t dirX, int8_t dirY);
    bool renderTile(const char* path, int16_t xOffset, int16_t yOffset, TFT_eSprite &map);
};

