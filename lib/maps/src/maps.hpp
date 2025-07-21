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
#include <unordered_map>
#include "tft.hpp"
#include "gpsMath.hpp"
#include "settings.hpp"
#include "compass.hpp"
#include "mapVars.h"
#include "storage.hpp"

/**********************************************************/
/**********************************************************/

#define CHECK_BOUNDS(offset, size, limit) \
    if (!checkBounds(offset, size, limit)) { \
        ESP_LOGE("BOUNDS", "Check failed: offset=%u size=%u limit=%u", (unsigned)(offset), (unsigned)(size), (unsigned)(limit)); \
        return false; \
    }

// Enhanced draw command types - SYNCHRONIZED WITH PYTHON SCRIPT
enum EnhancedDrawCommand : uint8_t 
{
    DRAW_LINE = 1,
    DRAW_POLYLINE = 2,
    DRAW_FILL_RECT = 3,
    DRAW_FILL_POLYGON = 4,
    DRAW_STROKE_POLYGON = 5,
    DRAW_ADAPTIVE_LINE = 6,     
    DRAW_SPLINE_CURVE = 7,      
    DRAW_MULTI_LOD_POLYGON = 8, 
    DRAW_HORIZONTAL_LINE = 9,
    DRAW_VERTICAL_LINE = 10,
};

// Complexity levels for geometry
enum GeometryComplexity : uint8_t 
{
    COMPLEXITY_LOW = 0,
    COMPLEXITY_MEDIUM = 1,
    COMPLEXITY_HIGH = 2,
};

// Cache structure for tile data
struct TileCache
{
    uint8_t* data;
    size_t size;
    uint32_t lastAccess;
    bool inPsram;
    
    TileCache();
    ~TileCache();
    
    // Prevent copying to avoid double-free issues
    TileCache(const TileCache&) = delete;
    TileCache& operator=(const TileCache&) = delete;
    
    // Allow moving
    TileCache(TileCache&& other) noexcept;
    TileCache& operator=(TileCache&& other) noexcept;
};

/**********************************************************/
/**********************************************************/

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


/**********************************************************/
/**********************************************************/
	static constexpr size_t MAX_CACHE_SIZE_PSRAM = 30;
    static constexpr size_t MAX_CACHE_SIZE_NO_PSRAM = 10;
    static constexpr size_t MAX_TILE_SIZE_BYTES = 1024 * 1024; // 1MB max per tile
    
    // Static members
    static std::unordered_map<std::string, TileCache> tileCache;
    static bool hasPsram;
    static size_t maxCacheSize;
    static bool debugMode;
    static int qualityLevel;
    static uint32_t cacheHits;
    static uint32_t cacheMisses;
    static uint32_t tilesRendered;
    static uint32_t commandsExecuted;
    
    // Command execution
    // static bool executeEnhancedCommand(uint8_t cmdType, const uint8_t* data, size_t& offset, 
    //                                   int16_t xOffset, int16_t yOffset, TFT_eSprite &map);
	static bool executeEnhancedCommand(uint8_t cmdType, const uint8_t* data, size_t& offset, 
                                 int16_t xOffset, int16_t yOffset, TFT_eSprite &map, size_t dataSize);
    
    // Basic drawing commands

    // Basic drawing commands
    static bool drawLine(const uint8_t* data, size_t& offset, 
                        int16_t xOffset, int16_t yOffset, TFT_eSprite &map);
    static bool drawPolyline(const uint8_t* data, size_t& offset, 
                            int16_t xOffset, int16_t yOffset, TFT_eSprite &map);
    static bool drawFillRect(const uint8_t* data, size_t& offset, 
                            int16_t xOffset, int16_t yOffset, TFT_eSprite &map);
    static bool drawFillPolygon(const uint8_t* data, size_t& offset, 
                               int16_t xOffset, int16_t yOffset, TFT_eSprite &map);
    static bool drawStrokePolygon(const uint8_t* data, size_t& offset, 
                                 int16_t xOffset, int16_t yOffset, TFT_eSprite &map);
    static bool drawHorizontalLine(const uint8_t* data, size_t& offset, 
                                  int16_t xOffset, int16_t yOffset, TFT_eSprite &map);
    static bool drawVerticalLine(const uint8_t* data, size_t& offset, 
                                int16_t xOffset, int16_t yOffset, TFT_eSprite &map);
    
    // Enhanced drawing commands
    static void drawAdaptiveLine(const int16_t* points_x, const int16_t* points_y, 
                               size_t count, uint16_t color, GeometryComplexity complexity, 
                               TFT_eSprite &map);
    static void drawMultiLODPolygon(const int16_t* points_x, const int16_t* points_y, 
                                  size_t count, uint16_t color, GeometryComplexity complexity, 
                                  uint8_t render_style, TFT_eSprite &map);
    static void drawSmoothCurve(const int16_t* points_x, const int16_t* points_y, 
                              size_t count, uint16_t color, TFT_eSprite &map);
    
    // Polygon filling utility
    static void drawPolygonFast(const int16_t* points_x, const int16_t* points_y, 
                               size_t count, uint16_t color, TFT_eSprite &map);
    
    // Utility functions
    static bool validateTileData(const uint8_t* data, size_t size);
    static void debugPrintCommand(uint8_t cmdType, GeometryComplexity complexity);
    static bool isVisible(int16_t x, int16_t y, int16_t w, int16_t h, const TFT_eSprite &map);
    static bool skipUnknownCommand(uint8_t cmdType, const uint8_t* data, size_t& offset, size_t dataSize);
    
    // Interpolation helpers
    static void interpolatePoints(const int16_t* points_x, const int16_t* points_y, 
                                size_t count, std::vector<int16_t>& out_x, std::vector<int16_t>& out_y);
    static void catmullRomSpline(const int16_t* points_x, const int16_t* points_y, 
                               size_t count, std::vector<int16_t>& out_x, std::vector<int16_t>& out_y);

    // Memory management helpers
    static void evictOldestTile();
    static bool loadTileFromFile(const char* path, TileCache& cache);
    
    // Bounds checking macros as inline functions
    static inline bool checkBounds(size_t offset, size_t size, size_t limit) {
        return (offset + size) <= limit;
    }
    
    static inline int16_t clampCoord(int32_t coord) {
		
        return static_cast<int16_t>(std::max(static_cast<int32_t>(-32767), 
                                           std::min(static_cast<int32_t>(32767), coord)));
    }

/**********************************************************/
/**********************************************************/

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




/**********************************************************/
/**********************************************************/
    // Core rendering functions
    static bool renderTile(const char* path, int16_t xOffset, int16_t yOffset, TFT_eSprite &map);
    
    // Cache management
    static void initCache();
    static void clearCache();
    static void printCacheStats();
    
    // Configuration
    static void setDebugMode(bool enabled);
    static void setQualityLevel(int level); // 1=fast, 2=balanced, 3=quality
    
    // Utility functions
    static size_t getCacheUsage();
    static bool hasPSRAM();
/**********************************************************/
/**********************************************************/

};

/**********************************************************/
/**********************************************************/
// Inline implementations for performance-critical functions
inline bool Maps::isVisible(int16_t x, int16_t y, int16_t w, int16_t h, const TFT_eSprite &map) {
    return !(x >= map.width() || y >= map.height() || x + w < 0 || y + h < 0);
}

inline bool Maps::hasPSRAM() {
    return hasPsram;
}

/**********************************************************/
/**********************************************************/