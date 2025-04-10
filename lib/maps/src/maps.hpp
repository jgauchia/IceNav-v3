/**
 * @file maps.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com) - Render Maps
 * @author @aresta - https://github.com/aresta/ESP32_GPS - Vector Maps
 * @brief  Maps draw class
 * @version 0.2.0
 * @date 2025-04
 */

#ifndef MAPS_HPP
#define MAPS_HPP

#include <cstdint>
#include <vector>
#include <map>
#include "tft.hpp"
#include "gpsMath.hpp"
#include "settings.hpp"
#include "compass.hpp"
#include "mapVars.h"
#include "storage.hpp"

class Maps
{
  private:  
    // Render Map
    struct MapTile                                                        // Tile Map structure
    {
        char *file;
        uint32_t tilex;
        uint32_t tiley;
        uint8_t zoom;
    };
    MapTile oldMapTile;                                                   // Old Map tile coordinates and zoom
    MapTile currentMapTile;                                               // Current Map tile coordinates and zoom
    MapTile roundMapTile;                                                 // Boundaries Map tiles
    uint16_t lon2posx(float f_lon, uint8_t zoom, uint16_t tileSize);
    uint16_t lat2posy(float f_lat, uint8_t zoom, uint16_t tileSize);
    uint32_t lon2tilex(double f_lon, uint8_t zoom);
    uint32_t lat2tiley(double f_lat, uint8_t zoom);
    double tilex2lon(uint32_t tileX, uint8_t zoom);
    double tiley2lat(uint32_t tileY, uint8_t zoom);
    MapTile getMapTile(double lon, double lat, uint8_t zoomLevel, int16_t offsetX, int16_t offsetY);

    // Vector Map  
    static const int32_t MAPBLOCK_MASK = pow(2, MAPBLOCK_SIZE_BITS) - 1;  // ...00000000111111111111
    static const int32_t MAPFOLDER_MASK = pow(2, MAPFOLDER_SIZE_BITS) - 1;// ...00001111
    struct Coord                                                          // Point in geographic (lat,lon) coordinates 
    {
      Point32 getPoint32();
      double lat = 0;
      double lng = 0;
    };
    struct Polyline                                                       // Polyline struct
    {
      std::vector<Point16> points;
      BBox bbox;
      uint16_t color;
      uint8_t width;
      uint8_t maxZoom;
    }; 
    struct Polygon                                                        // Polygon struct
    {
      std::vector<Point16> points;
      BBox bbox;
      uint16_t color;
      uint8_t maxZoom;
    };
    struct MapBlock                                                       // Map square area of aprox 4096 meters side. Correspond to one single map file.
    {
      Point32 offset;
      bool inView = false;
      std::vector<Polyline> polylines;
      std::vector<Polygon> polygons;
    };
    struct ViewPort                                                       // Vector map viewport structure
    {
      void setCenter(Point32 pcenter);
      Point32 center;
      BBox bbox;
    };
    ViewPort viewPort;                                                    // Vector map viewport
    struct MemBlocks                                                      // MemBlocks stored in memory
    {
      std::map<String, u_int16_t> blocks_map; // block offset -> block index
      std::array<MapBlock *, MAPBLOCKS_MAX> blocks;
    };
    MemBlocks memBlocks;                                                  // Vector file map memory blocks
    struct MemCache                                                       // MapBlocks memory store
    {
      std::vector<MapBlock *> blocks;
    };
    MemCache memCache;                                                    // Memory Cache
    Point32 point = viewPort.center;                                      // Vector map GPS position point
    double lat2y(double lat);
    double lon2x(double lon);
    double mercatorX2lon(double x);
    double mercatorY2lat(double y);
    int16_t toScreenCoord(const int32_t pxy, const int32_t screenCenterxy);
    uint32_t idx;
    int16_t parseInt16(char *file);
    void parseStrUntil(char *file, char terminator, char *str);
    void parseCoords(char *file, std::vector<Point16> &points);
    BBox parseBbox(String str);
    MapBlock *readMapBlock(String fileName);
    void fillPolygon(Polygon p, TFT_eSprite &map);
    void getMapBlocks(BBox &bbox, MemCache &memCache);
    void readVectorMap(ViewPort &viewPort, MemCache &memCache, TFT_eSprite &map, uint8_t zoom);
    void getPosition(double lat, double lon);

    // Common
    static const uint16_t tileHeight = 768;                               // Tile 9x9 Height Size
    static const uint16_t tileWidth = 768;                                // Tile 9x9 Width Size
    static const uint16_t renderMapTileSize = 256;                        // Render map tile size
    static const uint16_t vectorMapTileSize = tileHeight / 2;             // Vector map tile size
    uint16_t mapTileSize;                                                 // Actual map tile size (render or vector map)
    uint16_t wptPosX, wptPosY;                                            // Waypoint position on screen map
    TFT_eSprite arrowSprite = TFT_eSprite(&tft);                          // Sprite for Navigation Arrow in map
    TFT_eSprite mapTempSprite = TFT_eSprite(&tft);                        // Full map sprite (not showed)
    TFT_eSprite mapSprite = TFT_eSprite(&tft);                            // Screen map sprite (showed)
    double prevLat, prevLon;                                              // Previous Latitude and Longitude
    double destLat, destLon;                                              // Waypoint destination latitude and longitude
    uint8_t zoomLevel;                                                    // Zoom level for map display
    bool isMapFound = false;                                              // Flag to indicate when map is found on SD
    struct tileBounds                                                     // Map boundaries structure
    {
      double lat_min; 
      double lat_max; 
      double lon_min; 
      double lon_max; 
    };
    tileBounds totalBounds;                                               // Map boundaries
    struct ScreenCoord                                                    // Screen postion from GPS coordinates
    {
      uint16_t posX;
      uint16_t posY;
    };
    ScreenCoord navArrowPosition;                                         // Navigation Arrow position on screen
    tileBounds getTileBounds(uint32_t tileX, uint32_t tileY, uint8_t zoom);
    bool isCoordInBounds(double lat, double lon, tileBounds bound);
    ScreenCoord coord2ScreenPos(double lon, double lat, uint8_t zoomLevel, uint16_t tileSize);
    void coords2map(double lat, double lon, tileBounds bound, uint16_t *pixelX, uint16_t *pixelY);
    void showNoMap(TFT_eSprite &map);
    void drawMapWidgets(MAP mapSet);

  public:
    uint16_t mapScrHeight;                                               // Screen map size height   
    uint16_t mapScrWidth;                                                // Screen map size width
    uint16_t mapScrFull;                                                 // Screen map size in full screen
    bool redrawMap = true;                                               // Flag to indicate need redraw Map
    bool isPosMoved = true;                                              // Flag when current position changes (vector map)

    Maps();
    void initMap(uint16_t mapHeight, uint16_t mapWidth, uint16_t mapFull);
    void deleteMapScrSprites();
    void createMapScrSprites();
    void generateRenderMap(uint8_t zoom);
    void generateVectorMap(uint8_t zoom);
    void displayMap();
    void setWaypoint(double wptLat, double wptLon);
};

#endif