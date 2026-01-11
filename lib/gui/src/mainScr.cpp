/**
 * @file mainScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Main Screen
 * @version 0.2.4
 * @date 2025-12
 */

#include "mainScr.hpp"
#include "tasks.hpp"

bool isMainScreen = false;    
bool isScrolled = true;      
bool isReady = false;        
bool isScrollingMap = false;  
bool canScrollMap = false;   
uint8_t activeTile = 0;      
uint8_t gpxAction = WPT_NONE;
int heading = 0;              

extern uint32_t DOUBLE_TOUCH_EVENT; /**< Event code for double touch gesture */

extern Compass compass;
extern Gps gps;
extern wayPoint loadWpt;

#ifdef LARGE_SCREEN
    uint8_t toolBarOffset = 100;  /**< Toolbar offset for large screens */
    uint8_t toolBarSpace = 60;    /**< Toolbar spacing for large screens */
#else
    uint8_t toolBarOffset = 80;   /**< Toolbar offset for standard screens */
    uint8_t toolBarSpace = 50;    /**< Toolbar spacing for standard screens */
#endif

lv_obj_t *tilesScreen;
lv_obj_t *compassTile;
lv_obj_t *navTile;
lv_obj_t *mapTile;
lv_obj_t *satTrackTile;
lv_obj_t *btnZoomIn;
lv_obj_t *btnZoomOut;

lv_obj_t *mapCanvas;          /**< LVGL for the map canvas */

extern Maps mapView;

/**
 * @brief Update compass screen event
 *
 * @details Updates the compass screen UI elements (heading, coordinates, altitude, speed, sunrise/sunset) with current GPS and heading data when the relevant event is triggered.
 *
 * @param event LVGL event pointer.
 */
void updateCompassScr(lv_event_t *event)
{
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(event);
    if (obj == compassHeading)
    {
        lv_label_set_text_fmt(compassHeading, "%5d\xC2\xB0", heading);
        lv_img_set_angle(compassImg, -(heading * 10));
    }
    if (obj == latitude)
        lv_label_set_text_fmt(latitude, "%s", latFormatString(gps.gpsData.latitude));
    if (obj == longitude)
        lv_label_set_text_fmt(longitude, "%s", lonFormatString(gps.gpsData.longitude));
    if (obj == altitude)
        lv_label_set_text_fmt(obj, "%4d m.", gps.gpsData.altitude);
    if (obj == speedLabel)
        lv_label_set_text_fmt(obj, "%3d Km/h", gps.gpsData.speed);
    if (obj == sunriseLabel)
    {
        lv_label_set_text_static(obj, gps.gpsData.sunriseHour);
        lv_label_set_text_static(sunsetLabel, gps.gpsData.sunsetHour);
    }
}

/**
 * @brief Show Map Widgets.
 *
 * @details Displays or hides map-related UI widgets based on map user settings 
 */
void showMapWidgets()
{
    lv_obj_clear_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(zoomWidget, LV_OBJ_FLAG_HIDDEN);
    if (mapSet.showMapSpeed)
        lv_obj_clear_flag(mapSpeed,LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(mapSpeed,LV_OBJ_FLAG_HIDDEN);
    if (mapSet.showMapCompass)
        lv_obj_clear_flag(miniCompass,LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(miniCompass,LV_OBJ_FLAG_HIDDEN);
    if (mapSet.showMapScale)
        lv_obj_clear_flag(scaleWidget,LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(scaleWidget,LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief Hide Map Widgets.
 *
 * @details Hides all map-related UI widgets on the screen.
 */
void hideMapWidgets()
{
    lv_obj_add_flag(navArrow, LV_OBJ_FLAG_HIDDEN);  
    lv_obj_add_flag(zoomWidget, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mapSpeed,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(miniCompass,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(scaleWidget,LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief Get the active tile
 *
 * @details Handles the tileview scroll event, updates the active tile index, and manages map/widget visibility and bar status.
 *
 * @param event LVGL event pointer.
 */
void getActTile(lv_event_t *event)
{
    if (isReady)
    {
        isScrolled = true;
        mapView.redrawMap = true;

        if (activeTile == MAP)
        {
            mapView.createMapScrSprites();
            if (mapView.isMapFound)
                showMapWidgets();
            else
                hideMapWidgets();
        }

        if (isBarOpen)
        lv_obj_clear_flag(buttonBar, LV_OBJ_FLAG_HIDDEN);
    }
    else
        isReady = true;

    lv_obj_t *actTile = lv_tileview_get_tile_act(tilesScreen);
    lv_coord_t tileX = lv_obj_get_x(actTile) / TFT_WIDTH;
    activeTile = tileX;
}

/**
 * @brief Tile start scrolling event
 *
 * @details Handles the beginning of a tile scroll event by resetting scroll and map redraw flags and deleting map screen sprites.
 *
 * @param event LVGL event pointer.
 */
void scrollTile(lv_event_t *event)
{
    isScrolled = false;
    isReady = false;
    mapView.redrawMap = false;
    mapView.deleteMapScrSprites();
}

/**
 * @brief Update Main Screen.
 *
 * @details Periodically updates the active main screen tiles and its widgets 
 */
void updateMainScreen(lv_timer_t *t)
{
    if (isScrolled && isMainScreen)
    {
        #ifdef ENABLE_COMPASS
            if (!waitScreenRefresh)
                heading = globalSensorData.heading;
        #else
            heading = gps.gpsData.heading;
        #endif

        if (gps.hasLocationChange())
        {
            lv_obj_send_event(latitude, LV_EVENT_VALUE_CHANGED, NULL);
            lv_obj_send_event(longitude, LV_EVENT_VALUE_CHANGED, NULL);
        }

        switch (activeTile)
        {
        case COMPASS:
            static int lastHeadingComp = -1;
            if (heading != lastHeadingComp)
            {
                lastHeadingComp = heading;
                lv_obj_send_event(compassHeading, LV_EVENT_VALUE_CHANGED, NULL);
            }

            static int16_t lastAltMain = -32768;
            if (gps.gpsData.altitude != lastAltMain)
            {
                lastAltMain = gps.gpsData.altitude;
                lv_obj_send_event(altitude, LV_EVENT_VALUE_CHANGED, NULL);
            }

            if (gps.isSpeedChanged())
                lv_obj_send_event(speedLabel, LV_EVENT_VALUE_CHANGED, NULL);
            break;

        case MAP:
            lv_obj_send_event(mapTile, LV_EVENT_VALUE_CHANGED, NULL);
            break;

        case NAV:
            lv_obj_send_event(navTile, LV_EVENT_VALUE_CHANGED, NULL);
            break;

        case SATTRACK:
            lv_obj_send_event(satTrackTile, LV_EVENT_VALUE_CHANGED, NULL);
            break;

        default:
            break;
        }
    }
}

/**
 * @brief Update map event
 *
 * @details Handles map update events by generating and displaying the map (vector or render), updating map speed, scale, and compass widgets according to current settings.
 *
 * @param event LVGL event pointer.
 */
void updateMap(lv_event_t *event)
{
    mapView.generateMap(zoom);

    if (mapView.redrawMap)
    {
        mapView.displayMap();
        // Virtual canvas for smooth panning
        lv_canvas_set_buffer(mapCanvas, mapView.mapBuffer, Maps::tileWidth, Maps::tileHeight, LV_COLOR_FORMAT_RGB565_SWAPPED);
    }

    // Position canvas for viewport
    int16_t baseX = (tft.width() - Maps::tileWidth) / 2;
    int16_t baseY = ((tft.height() - 27) - Maps::tileHeight) / 2;
    if (mapView.followGps)
    {
        // GPS mode: center is already correct from pushRotated, no offset needed
        lv_obj_set_pos(mapCanvas, baseX, baseY);
    }
    else
    {
        // Manual mode: apply panning offsets for smooth scroll
        lv_obj_set_pos(mapCanvas, baseX - mapView.offsetX, baseY - mapView.offsetY);
    }

    if (mapSet.showMapSpeed)
        lv_label_set_text_fmt(mapSpeedLabel, "%3d", gps.gpsData.speed);     
    
    if (mapSet.showMapScale)
        lv_label_set_text_fmt(scaleLabel, "%s", map_scale[zoom]);

    if (mapSet.showMapCompass)
    {
        if (mapSet.compassRotation)
        lv_img_set_angle(mapCompassImg, -(heading * 10));
    }
}

/**
 * @brief Update Satellite Tracking.
 *
 * @details Handles satellite tracking update events by refreshing DOP, altitude labels, and updating satellite SNR and sky plots.
 */
void updateSatTrack(lv_event_t *event)
{
    if (gps.isDOPChanged())
    {
        lv_label_set_text_fmt(pdopLabel, "PDOP: %.1f", gps.gpsData.pdop);
        lv_label_set_text_fmt(hdopLabel, "HDOP: %.1f", gps.gpsData.hdop);
        lv_label_set_text_fmt(vdopLabel, "VDOP: %.1f", gps.gpsData.vdop);
    }

    static int16_t lastAltSat = -32768;
    if (gps.gpsData.altitude != lastAltSat)
    {
        lastAltSat = gps.gpsData.altitude;
        lv_label_set_text_fmt(altLabel, "ALT: %4dm.", gps.gpsData.altitude);
    }

    drawSatSNR();
    drawSatSky();
}

/**
 * @brief Map Tool Bar Event.
 *
 * @details Handles map toolbar visibility toggling, zoom button states, map scrollability, and map centering on GPS.
 *
 * @param event LVGL event pointer.
 */
void mapToolBarEvent(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);

    showMapToolBar = !showMapToolBar;
    canScrollMap = !canScrollMap;

    if (!showMapToolBar)
    {
        lv_obj_clear_flag(btnZoomOut, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
        lv_obj_clear_flag(btnZoomIn, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
        lv_obj_add_flag(btnZoomOut, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(btnZoomIn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(tilesScreen, LV_OBJ_FLAG_SCROLLABLE);
        mapView.centerOnGps(gps.gpsData.latitude, gps.gpsData.longitude);
        lv_obj_clear_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(btnZoomOut, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
        lv_obj_add_flag(btnZoomIn, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
        lv_obj_clear_flag(btnZoomOut, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(btnZoomIn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(tilesScreen, LV_OBJ_FLAG_SCROLLABLE);
        if (!mapView.followGps)
            lv_obj_add_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_clear_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * @brief Scroll Map Event.
 *
 * @details Handles map scrolling gestures, updating the map view position 
 *
 * @param event LVGL event pointer.
 */
void scrollMapEvent(lv_event_t *event)
{
    if (canScrollMap)
    {
        lv_event_code_t code = lv_event_get_code(event);
        lv_indev_t * indev = lv_event_get_indev(event);
        static int last_x = 0, last_y = 0;
        static int dx = 0, dy = 0;
        lv_point_t p;


        switch (code)
        {
            case LV_EVENT_PRESSED:
            {
                lv_indev_get_point(indev, &p);
                last_x = p.x;
                last_y = p.y;
                isScrollingMap = true;
                break;
            }
        
            case LV_EVENT_PRESSING:
            {
                lv_indev_get_point(indev, &p);

                int dx = p.x - last_x;
                int dy = p.y - last_y;

                const int SCROLL_THRESHOLD = 5; 

                if (abs(dx) > SCROLL_THRESHOLD || abs(dy) > SCROLL_THRESHOLD) 
                {
                    mapView.scrollMap(-dx, -dy);
                    lv_obj_add_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
                    last_x = p.x;
                    last_y = p.y;
                }
                break;
            }
        
            case LV_EVENT_PRESS_LOST:
            {
                lv_obj_clear_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
                isScrollingMap = false;
                break;
            }
        }
    }
}

/**
 * @brief Zoom Event Toolbar.
 *
 * @details Handles zoom in/out toolbar events, updates zoom level, manages map position, and refreshes the map
 *
 * @param event LVGL event pointer.
 */
void zoomEvent(lv_event_t *event)
{
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(event);
    if ( obj == btnZoomIn && ( zoom >= minZoom && zoom < maxZoom ) )
        zoom++;
    else if ( obj == btnZoomOut && ( zoom <= maxZoom && zoom > minZoom ) )
        zoom--;
    lv_obj_send_event(mapTile, LV_EVENT_REFRESH, NULL);
    lv_label_set_text_fmt(zoomLabel, "%2d", zoom);
}

/**
 * @brief Handles navigation waypoint screen updates 
 *
 * @param event LVGL event pointer.
 */
void updateNavEvent(lv_event_t *event)
{
    int wptDistance = (int)calcDist(gps.gpsData.latitude, gps.gpsData.longitude, loadWpt.lat, loadWpt.lon);
    lv_label_set_text_fmt(distNav, "%d m.", wptDistance);

    if (wptDistance == 0)
    {
        LV_IMG_DECLARE(navfinish);
        lv_img_set_src(arrowNav, &navfinish);
        lv_img_set_angle(arrowNav, 0);
    }
    else
    {
        #ifdef ENABLE_COMPASS
            float wptCourse = calcCourse(gps.gpsData.latitude, gps.gpsData.longitude, loadWpt.lat, loadWpt.lon) - heading;
        #endif
        #ifndef ENABLE_COMPASS
            float wptCourse = calcCourse(gps.gpsData.latitude, gps.gpsData.longitude, loadWpt.lat, loadWpt.lon) - heading;
        #endif
            lv_img_set_angle(arrowNav, (wptCourse * 10));
    }
}

/**
 * @brief Create Canvas for Map.
 *
 * @details Initializes and creates the canvas object for rendering the map on the specified screen.
 *
 * @param screen Pointer to the LVGL screen object.
 */
void createMapCanvas(_lv_obj_t *screen)
{
    mapCanvas = lv_canvas_create(screen);
    // Hide scrollbars for virtual canvas
    lv_obj_set_scrollbar_mode(mapCanvas, LV_SCROLLBAR_MODE_OFF);
    // Prevent canvas from affecting parent scroll
    lv_obj_add_flag(mapCanvas, LV_OBJ_FLAG_FLOATING);
}

/**
 * @brief Create Main Screen.
 *
 * @details Initializes and configures the main screen and its tiles, widgets, and event callbacks 
 */
void createMainScr()
{
    mainScreen = lv_obj_create(NULL);

    // Main Screen Tiles
    tilesScreen = lv_tileview_create(mainScreen);
    compassTile = lv_tileview_add_tile(tilesScreen, 0, 0, LV_DIR_RIGHT);
    mapTile = lv_tileview_add_tile(tilesScreen, 1, 0, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_RIGHT));
    navTile = lv_tileview_add_tile(tilesScreen, 2, 0, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_RIGHT));
    lv_obj_add_flag(navTile, LV_OBJ_FLAG_HIDDEN);
    satTrackTile = lv_tileview_add_tile(tilesScreen, 3, 0, LV_DIR_LEFT);
    lv_obj_set_size(tilesScreen, TFT_WIDTH, TFT_HEIGHT - 25);
    lv_obj_set_pos(tilesScreen, 0, 25);
    lv_obj_add_style(tilesScreen, &styleScrollbarWhite, LV_PART_SCROLLBAR);
    // Main Screen Events
    lv_obj_add_event_cb(tilesScreen, getActTile, LV_EVENT_SCROLL_END, NULL);
    lv_obj_add_event_cb(tilesScreen, scrollTile, LV_EVENT_SCROLL_BEGIN, NULL);

    // ********** Compass Tile **********
    // Compass Widget
    compassWidget(compassTile);
    // Position widget
    positionWidget(compassTile);
    // Altitude widget
    altitudeWidget(compassTile);
    // Speed widget
    speedWidget(compassTile);
    // Sunrise/Sunset widget
    sunWidget(compassTile);
    // Compass Tile Events
    lv_obj_add_event_cb(compassHeading, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(latitude, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(longitude, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(altitude, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(speedLabel, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(sunriseLabel, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(sunsetLabel, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);

    // ********** Map Tile **********
    // Map Canvas 
    createMapCanvas(mapTile);
    // Navigation Arrow Widget
    navArrowWidget(mapTile);
    // Map zoom Widget
    mapZoomWidget(mapTile);
    // Map speed Widget
    mapSpeedWidget(mapTile);
    // Map compass Widget
    mapCompassWidget(mapTile);
    // Map scale Widget
    mapScaleWidget(mapTile);
    // Turn by Turn navigation widget
      turnByTurnWidget(mapTile);
    // Map Tile Toolbar
    btnZoomOut = lv_img_create(mapTile);
    lv_img_set_src(btnZoomOut, zoomOutIconFile);
    lv_img_set_zoom(btnZoomOut,buttonScale);
    lv_obj_update_layout(btnZoomOut);
    lv_obj_set_size(btnZoomOut,  48 * scaleBut, 48 * scaleBut);
    btnZoomIn = lv_img_create(mapTile);
    lv_img_set_src(btnZoomIn, zoomInIconFile);
    lv_img_set_zoom(btnZoomIn,buttonScale);
    lv_obj_update_layout(btnZoomIn);
    lv_obj_set_size(btnZoomIn,  48 * scaleBut, 48 * scaleBut);
    lv_obj_set_pos(btnZoomOut, 10, mapView.mapScrHeight - toolBarOffset);
    lv_obj_set_pos(btnZoomIn, 10, mapView.mapScrHeight - (toolBarOffset + toolBarSpace));
    if (!showMapToolBar)
    {
        lv_obj_clear_flag(btnZoomOut, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
        lv_obj_clear_flag(btnZoomIn, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
        lv_obj_add_flag(btnZoomOut, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(btnZoomIn, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(btnZoomOut, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
        lv_obj_add_flag(btnZoomIn, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
        lv_obj_clear_flag(btnZoomOut, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(btnZoomIn, LV_OBJ_FLAG_HIDDEN);
    }
    // Map Tile Events
    lv_obj_add_event_cb(mapTile, updateMap, LV_EVENT_VALUE_CHANGED, NULL);
    DOUBLE_TOUCH_EVENT = lv_event_register_id();
    lv_obj_add_event_cb(mapTile, mapToolBarEvent, (lv_event_code_t)DOUBLE_TOUCH_EVENT, NULL);
    lv_obj_add_event_cb(mapTile, scrollMapEvent, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(btnZoomOut, zoomEvent, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btnZoomIn, zoomEvent, LV_EVENT_CLICKED, NULL);

    // ********** Navigation Tile **********
    navigationScr(navTile);
    // Navigation Tile Events
    lv_obj_add_event_cb(navTile, updateNavEvent, LV_EVENT_VALUE_CHANGED, NULL);

    // ********** Satellite Tracking and info Tile **********
    satelliteScr(satTrackTile);
    #ifdef BOARD_HAS_PSRAM
        #ifndef TDECK_ESP32S3
            createConstCanvas(satTrackTile);
            drawSatConst();
            lv_obj_set_pos(constCanvas, (TFT_WIDTH / 2) - canvasCenter_X, 240);
        #endif
        #ifdef TDECK_ESP32S3
            createConstCanvas(constMsg);
            lv_obj_align(constCanvas, LV_ALIGN_CENTER, 0, 0);
            drawSatConst();
        #endif
    #endif
    // Satellite Tracking Event
    lv_obj_add_event_cb(satTrackTile, updateSatTrack, LV_EVENT_VALUE_CHANGED, NULL);
}
