/**
 * @file maps.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) - Render Maps
 * @brief  Maps draw class
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "tft.hpp"
#include "gpsMath.hpp"
#include "settings.hpp"
#include "compass.hpp"
#include "mapVars.h"
#include "storage.hpp"
#include "nav_reader.hpp"
#include "InternalAllocator.hpp"

/**
 * @class Maps
 * @brief Maps class for handling vector and raster map rendering, navigation, and viewport management.
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

        static const uint16_t mapTileSize = 256;                             	     /**< Map tile size */

        tileBounds totalBounds; 													/**< Map boundaries */
        uint16_t wptPosX, wptPosY;                                                  /**< Waypoint position on screen map */
        TFT_eSprite mapTempSprite = TFT_eSprite(&tft);                              /**< Full map sprite (not showed) */
        TFT_eSprite mapSprite = TFT_eSprite(&tft);                                  /**< Screen map sprite (showed) */
        TFT_eSprite preloadSprite = TFT_eSprite(&tft);                              /**< Preload sprite for scroll (reusable) */
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
        uint16_t darkenRGB565(const uint16_t color, const float amount = 0.4f);  					/**< Darken RGB565 color by a given fraction */
        void fillPolygonGeneral(TFT_eSprite &map, const int *px, const int *py, 
                                const int numPoints, const uint16_t color,	
                                const int xOffset, const int yOffset,
                                uint16_t ringCount = 1, uint16_t* ringEnds = nullptr);						/**< Fill a polygon using the scanline algorithm and multi-ring support */

    public:
        // Virtual canvas dimensions (public for external access)
        static const uint16_t tileWidth = 768;                                      /**< Virtual canvas width */
        static const uint16_t tileHeight = 768;                                     /**< Virtual canvas height */

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

        // NAV tile rendering methods
        bool renderNavViewport(float centerLat, float centerLon, uint8_t zoom, TFT_eSprite &map);
        void renderNavTile(uint32_t tileX, uint32_t tileY, uint8_t zoom, int16_t screenX, int16_t screenY, TFT_eSprite &map);

    private:
        // NAV tile rendering helpers
        void renderNavFeature(const NavFeature& feature, const NavBbox& viewport, TFT_eSprite& map);
        void renderNavLineString(const NavFeature& feature, TFT_eSprite& map);
        void renderNavPolygon(const NavFeature& feature, TFT_eSprite& map);
        void renderNavPoint(const NavFeature& feature, TFT_eSprite& map);
        void navCoordToPixel(const NavFeature& feature, const NavCoord& coord, int16_t& px, int16_t& py);
        void latLonToPixel(float lat, float lon, int16_t& px, int16_t& py);
        void drawTrack(TFT_eSprite &map);

    public:
        bool trackNeedsRedraw = false;                                              /**< Flag to indicate if track needs immediate redraw */
        void redrawTrack();
        bool isRendering() const { return !pendingTiles.empty(); }                  /**< Check if background rendering is active */

    private:
        enum TileType
        {
            TILE_NAV,
            TILE_PNG
        };

        struct PendingTile
        {
            uint32_t x;                                                             /**< Tile X coordinate */
            uint32_t y;                                                             /**< Tile Y coordinate */
            int16_t screenX;                                                        /**< X position in sprite */
            int16_t screenY;                                                        /**< Y position in sprite */
            TileType type;                                                          /**< Type of tile (NAV or PNG) */
        };
        std::vector<PendingTile> pendingTiles;                                      /**< Queue for asynchronous tile rendering */
        SemaphoreHandle_t mapMutex;                                                 /**< Mutex for thread-safe access */
        TaskHandle_t mapRenderTaskHandle;                                           /**< Handle for the background render task */
        static void mapRenderTask(void* pvParameters);                              /**< Background task function */

        // NAV tile rendering methods
        void renderPngTile(uint32_t tileX, uint32_t tileY, uint8_t zoom, int16_t screenX, int16_t screenY, TFT_eSprite &map);

        // NAV render state
        float navLastLat_;
        float navLastLon_;
        uint8_t navLastZoom_;
        bool navNeedsRender_;
        
        // Viewport top-left in tile units for arbitrary projection (GPX tracks)
        float navTlTileX_;
        float navTlTileY_;

        // Physics for Natural Scroll
    public:
        float velocityX = 0.0f;                                                     /**< Current horizontal velocity (px/ms) */
        float velocityY = 0.0f;                                                     /**< Current vertical velocity (px/ms) */
        const float friction = 0.95f;                                               /**< Friction factor for inertia deceleration */

    private:
        // Zero-Allocation Projection Pipeline
        std::vector<int16_t, InternalAllocator<int16_t>> projBuf16X;
        std::vector<int16_t, InternalAllocator<int16_t>> projBuf16Y;
        std::vector<int, InternalAllocator<int>> projBuf32X;
        std::vector<int, InternalAllocator<int>> projBuf32Y;

        // AEL Polygon optimization
        struct Edge {
            int yMax;
            int xVal;       // Fixed point 16.16
            int slope;      // Fixed point 16.16
            int nextInBucket; 
            int nextActive;
        };
        std::vector<int, InternalAllocator<int>> edgeBuckets;
        std::vector<Edge, InternalAllocator<Edge>> edgePool;
};