/**
 * @file mainScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Main Screen
 * @version 0.2.5
 * @date 2026-04
 */

#include "mainScr.hpp"
#include "tasks.hpp"
#include "lv_subjects.hpp"

#define MAP_MODE_FOLLOW 0
#define MAP_MODE_MANUAL 1
#define MAP_MODE_INERTIA 2

bool isMainScreen = false;
bool isScrolled = true;      
bool isScrollingMap = false;  
bool canScrollMap = false;   
uint8_t activeTile = 0;
uint8_t gpxAction = WPT_NONE;

lv_timer_t *map_inertia_timer = NULL;

extern uint32_t DOUBLE_TOUCH_EVENT;extern Compass compass;
extern Gps gps;
extern wayPoint loadWpt;

#ifdef LARGE_SCREEN
    uint8_t toolBarOffset = 100;
    uint8_t toolBarSpace = 60;
#else
    uint8_t toolBarOffset = 80;
    uint8_t toolBarSpace = 50;
#endif

lv_obj_t *tilesScreen;
lv_obj_t *compassTile;
lv_obj_t *navTile;
lv_obj_t *mapTile;
lv_obj_t *satTrackTile;
lv_obj_t *btnZoomIn;
lv_obj_t *btnZoomOut;
lv_obj_t *mapCanvas;
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
        lv_obj_clear_flag(mapSpeed, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(mapSpeed, LV_OBJ_FLAG_HIDDEN);
    if (mapSet.showMapCompass)
        lv_obj_clear_flag(miniCompass, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(miniCompass, LV_OBJ_FLAG_HIDDEN);
    if (mapSet.showMapScale)
        lv_obj_clear_flag(scaleWidget, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(scaleWidget, LV_OBJ_FLAG_HIDDEN);
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
    lv_obj_add_flag(mapSpeed, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(miniCompass, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(scaleWidget, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief Global heading state for map observer filtering.
 */
static int global_last_heading = -1;

/**
 * @brief Async callback to delegate map redrawing to UI thread (Core 1)
 */
static void async_map_update_cb(void * user_data)
{
    if (mapTile != NULL)
        lv_obj_send_event(mapTile, LV_EVENT_VALUE_CHANGED, NULL);
}

/**
 * @brief Observer callback for map position updates (GPS)
 *
 * @details Triggers map redraws automatically when position changes,
 *          but only if Follow GPS mode is active. Uses lv_async_call
 *          to delegate rendering to the UI thread (Core 1).
 *
 * @param observer Pointer to the observer.
 * @param subject Pointer to the subject.
 */
static void map_position_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    if (activeTile != MAP || lv_subject_get_int(&subject_map_state) != MAP_MODE_FOLLOW)
        return;

    lv_async_call(async_map_update_cb, NULL);
}

/**
 * @brief Observer callback for map heading updates (Compass or GPS)
 *
 * @details Triggers map redraws automatically when heading changes significantly,
 *          eliminating the need for manual polling in the timer. Uses lv_async_call
 *          to ensure heavy rendering happens safely on the UI thread (Core 1).
 *
 * @param observer Pointer to the observer.
 * @param subject Pointer to the subject.
 */
static void map_heading_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    if (activeTile != MAP || canMoveWidget || lv_subject_get_int(&subject_map_state) != MAP_MODE_FOLLOW)
        return;

    int32_t newHeading = lv_subject_get_int(subject);
    
    if (abs(newHeading - global_last_heading) > 1)
    {
        global_last_heading = newHeading;
        lv_async_call(async_map_update_cb, NULL);
    }
}

/**
 * @brief Async callback to delegate nav redrawing to UI thread (Core 1)
 */
static void async_nav_update_cb(void * user_data)
{
    if (navTile != NULL)
        lv_obj_send_event(navTile, LV_EVENT_VALUE_CHANGED, NULL);
}

/**
 * @brief Observer callback for nav data updates (Lat, Lon, Heading)
 *
 * @details Triggers nav screen redraws automatically when movement or rotation occurs.
 *          Uses lv_async_call to ensure rendering happens safely on the UI thread.
 *
 * @param observer Pointer to the observer.
 * @param subject Pointer to the subject.
 */
static void nav_data_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    if (activeTile != NAV)
        return;
    lv_async_call(async_nav_update_cb, NULL);
}

/**
 * @brief Get active tile
 *
 * @details Handles tileview scroll event, updates active tile index, and manages map/widget visibility and bar status.
 *
 * @param event LVGL event pointer.
 */
void getActTile(lv_event_t *event)
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
    lv_obj_t *actTile = lv_tileview_get_tile_act(tilesScreen);
    if (actTile == NULL) 
        return;
    activeTile = lv_obj_get_x(actTile) / TFT_WIDTH;
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
    if (isScrolled && isMainScreen || isScrollingMap)
    {
        switch (activeTile)
        {
            case SATTRACK:
                static uint8_t lastSats = 0;
                if (gps.isDOPChanged() || gps.hasLocationChange() || gps.gpsData.satInView != lastSats)
                {
                    lastSats = gps.gpsData.satInView;
                    lv_obj_send_event(satTrackTile, LV_EVENT_VALUE_CHANGED, NULL);
                }
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
    if (mapView.redrawMap && !mapSet.vectorMap)
        xEventGroupSetBits(mapView.mapEventGroup, Maps::MAP_EVENT_DONE);

    static int16_t lastDispX = -32768;
    static int16_t lastDispY = -32768;
    static int32_t lastRenderedHeading = -1;
    
    int32_t currentHeading = lv_subject_get_int(&subject_heading);
    bool headingChanged = (abs(currentHeading - lastRenderedHeading) > 1);

    if (mapView.offsetX != lastDispX || 
        mapView.offsetY != lastDispY || 
        (headingChanged && mapView.followGps) ||
        (xEventGroupGetBits(mapView.mapEventGroup) & Maps::MAP_EVENT_DONE))
    {
        lastDispX = mapView.offsetX;
        lastDispY = mapView.offsetY;
        lastRenderedHeading = currentHeading;
        xEventGroupClearBits(mapView.mapEventGroup, Maps::MAP_EVENT_DONE);
        mapView.displayMap();
        lv_canvas_set_buffer(mapCanvas, mapView.mapBuffer, mapView.mapScrWidth, mapView.mapScrHeight, LV_COLOR_FORMAT_RGB565_SWAPPED);
        mapView.redrawMap = false;
    }

    lv_obj_set_pos(mapCanvas, 0, 0);

    if (mapSet.showMapSpeed)
        lv_label_set_text_fmt(mapSpeedLabel, "%3d", gps.gpsData.speed);
    if (mapSet.showMapScale)
        lv_label_set_text_fmt(scaleLabel, "%s", map_scale[zoom]);
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
    showMapToolBar = !showMapToolBar;
    canScrollMap = !canScrollMap;
    isScrollingMap = false;
    if (!showMapToolBar)
    {
        lv_obj_clear_flag(btnZoomOut, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
        lv_obj_clear_flag(btnZoomIn, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
        lv_obj_add_flag(btnZoomOut, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(btnZoomIn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(tilesScreen, LV_OBJ_FLAG_SCROLLABLE);
        mapView.centerOnGps(gps.gpsData.latitude, gps.gpsData.longitude);
        lv_subject_set_int(&subject_map_state, MAP_MODE_FOLLOW);
        mapView.updateMap();
        lv_obj_clear_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
        lv_obj_send_event(mapTile, LV_EVENT_VALUE_CHANGED, NULL);
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
 * @brief Timer callback for map inertia motor.
 *
 * @details Calculates the inertia movement based on velocity and applies friction.
 *          Updates the map position and triggers redrawing.
 */
void map_inertia_timer_cb(lv_timer_t * t)
{
    float dt = 20.0f; // Fixed period defined in createMainScr()
    if (mapView.velocityX != 0 || mapView.velocityY != 0)
    {
        float dx = mapView.velocityX * dt;
        float dy = mapView.velocityY * dt;
        mapView.scrollMap((int16_t)dx, (int16_t)dy);

        float currentFriction = mapView.isRendering() ? 0.85f : mapView.friction;
        mapView.velocityX *= currentFriction;
        mapView.velocityY *= currentFriction;

        if (abs(mapView.velocityX) < 0.1f)
            mapView.velocityX = 0;
        if (abs(mapView.velocityY) < 0.1f)
            mapView.velocityY = 0;

        lv_obj_send_event(mapTile, LV_EVENT_VALUE_CHANGED, NULL);
    }
    else
    {
        lv_timer_pause(t);
        lv_subject_set_int(&subject_map_state, MAP_MODE_MANUAL);
    }
}

/**
 * @brief Scroll Map Event.
 *
 * @details Handles map scrolling gestures, updating the map view position and calculating
 *          real-time velocity (px/ms) for inertial movement.
 *
 * @param event LVGL event pointer.
 */
void scrollMapEvent(lv_event_t *event)
{
    if (canScrollMap)
    {
        lv_event_code_t code = lv_event_get_code(event);
        lv_indev_t * indev = lv_event_get_indev(event);
        static int last_x = 0;
        static int last_y = 0;
        static uint32_t last_time = 0;
        static bool dragStarted = false;
        lv_point_t p;

        switch (code)
        {
            case LV_EVENT_PRESSED:
                lv_indev_get_point(indev, &p);
                last_x = p.x;
                last_y = p.y;
                last_time = (uint32_t)(esp_timer_get_time() / 1000);
                dragStarted = false;
                isScrollingMap = true;
                mapView.velocityX = 0;
                mapView.velocityY = 0;
                lv_subject_set_int(&subject_map_state, MAP_MODE_MANUAL);
                if (map_inertia_timer != NULL)
                    lv_timer_pause(map_inertia_timer);
                break;

            case LV_EVENT_PRESSING:
            {
                lv_indev_get_point(indev, &p);
                uint32_t current_time = (uint32_t)(esp_timer_get_time() / 1000);
                int dx = p.x - last_x;
                int dy = p.y - last_y;
                uint32_t dt = current_time - last_time;

                if (!dragStarted)
                {
                    const int START_THRESHOLD = 12;
                    if (abs(dx) > START_THRESHOLD || abs(dy) > START_THRESHOLD)
                    {
                        dragStarted = true;
                        lv_obj_add_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
                    }
                }

                if (dragStarted && dt > 0)
                {
                    mapView.scrollMap(-dx, -dy);
                    float weight = 0.7f;
                    mapView.velocityX = mapView.velocityX * (1.0f - weight) + (-(float)dx / (float)dt) * weight;
                    mapView.velocityY = mapView.velocityY * (1.0f - weight) + (-(float)dy / (float)dt) * weight;
                    last_x = p.x;
                    last_y = p.y;
                    last_time = current_time;
                    lv_obj_send_event(mapTile, LV_EVENT_VALUE_CHANGED, NULL);
                }
                break;
            }
            case LV_EVENT_RELEASED:
            case LV_EVENT_PRESS_LOST:
                lv_obj_clear_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
                isScrollingMap = false;
                dragStarted = false;
                if (abs(mapView.velocityX) > 0.1f || abs(mapView.velocityY) > 0.1f)
                {
                    lv_subject_set_int(&subject_map_state, MAP_MODE_INERTIA);
                    if (map_inertia_timer != NULL)
                        lv_timer_resume(map_inertia_timer);
                }
                else
                {
                    mapView.velocityX = 0;
                    mapView.velocityY = 0;
                }
                break;
            default: 
                break;
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
    
    mapView.updateMap();
    lv_obj_send_event(mapTile, LV_EVENT_VALUE_CHANGED, NULL);
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
        float navHeading = (float)lv_subject_get_int(&subject_heading);
        float wptCourse = calcCourse(gps.gpsData.latitude, gps.gpsData.longitude, loadWpt.lat, loadWpt.lon) - navHeading;
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
    lv_obj_set_scrollbar_mode(mapCanvas, LV_SCROLLBAR_MODE_OFF);
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
    tilesScreen = lv_tileview_create(mainScreen);
    compassTile = lv_tileview_add_tile(tilesScreen, 0, 0, LV_DIR_RIGHT);
    mapTile = lv_tileview_add_tile(tilesScreen, 1, 0, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_RIGHT));
    navTile = lv_tileview_add_tile(tilesScreen, 2, 0, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_RIGHT));
    lv_obj_add_flag(navTile, LV_OBJ_FLAG_HIDDEN);
    satTrackTile = lv_tileview_add_tile(tilesScreen, 3, 0, LV_DIR_LEFT);
    lv_obj_set_size(tilesScreen, TFT_WIDTH, TFT_HEIGHT - 25);
    lv_obj_set_pos(tilesScreen, 0, 25);
    lv_obj_add_style(tilesScreen, &styleScrollbarWhite, LV_PART_SCROLLBAR);
    lv_obj_add_event_cb(tilesScreen, getActTile, LV_EVENT_SCROLL_END, NULL);
    lv_obj_add_event_cb(tilesScreen, scrollTile, LV_EVENT_SCROLL_BEGIN, NULL);
    lv_obj_add_event_cb(tilesScreen, getActTile, LV_EVENT_SCROLL, NULL);
    compassWidget(compassTile);
    positionWidget(compassTile);
    altitudeWidget(compassTile);
    speedWidget(compassTile);
    sunWidget(compassTile);
    lv_subject_add_observer_obj(&subject_heading, map_heading_observer_cb, mapTile, NULL);
    lv_subject_add_observer_obj(&subject_lat, map_position_observer_cb, mapTile, NULL);
    lv_subject_add_observer_obj(&subject_lon, map_position_observer_cb, mapTile, NULL);
    lv_obj_add_event_cb(sunriseLabel, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(sunsetLabel, updateCompassScr, LV_EVENT_VALUE_CHANGED, NULL);
    createMapCanvas(mapTile);
    navArrowWidget(mapTile);
    mapZoomWidget(mapTile);
    mapSpeedWidget(mapTile);
    mapCompassWidget(mapTile);
    mapScaleWidget(mapTile);
    turnByTurnWidget(mapTile);
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
    lv_obj_add_event_cb(mapTile, updateMap, LV_EVENT_VALUE_CHANGED, NULL);
    DOUBLE_TOUCH_EVENT = lv_event_register_id();
    lv_obj_add_event_cb(mapTile, mapToolBarEvent, (lv_event_code_t)DOUBLE_TOUCH_EVENT, NULL);
    lv_obj_add_event_cb(mapTile, scrollMapEvent, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(btnZoomOut, zoomEvent, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btnZoomIn, zoomEvent, LV_EVENT_CLICKED, NULL);
    navigationScr(navTile);
    lv_subject_add_observer_obj(&subject_lat, nav_data_observer_cb, navTile, NULL);
    lv_subject_add_observer_obj(&subject_lon, nav_data_observer_cb, navTile, NULL);
    lv_subject_add_observer_obj(&subject_heading, nav_data_observer_cb, navTile, NULL);
    lv_obj_add_event_cb(navTile, updateNavEvent, LV_EVENT_VALUE_CHANGED, NULL);
    satelliteScr(satTrackTile);
    map_inertia_timer = lv_timer_create(map_inertia_timer_cb, 20, NULL);
    lv_timer_pause(map_inertia_timer);
    
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
    lv_obj_add_event_cb(satTrackTile, updateSatTrack, LV_EVENT_VALUE_CHANGED, NULL);

    if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        lv_subject_set_int(&subject_lat, (int32_t)(gps.getLat() * 1000000.0f));
        lv_subject_set_int(&subject_lon, (int32_t)(gps.getLon() * 1000000.0f));
        xSemaphoreGive(lvgl_mutex);
    }
}
