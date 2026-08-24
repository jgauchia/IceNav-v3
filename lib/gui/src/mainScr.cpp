/**
 * @file mainScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Main Screen
 * @version 0.3.0
 * @date 2026-06
 */

#include "mainScr.hpp"
#include "tasks.hpp"
#include "lv_subjects.hpp"
#include "navContext.hpp"
#include "logger.hpp"

#define MAP_MODE_FOLLOW 0
#define MAP_MODE_MANUAL 1
#define MAP_MODE_INERTIA 2

static const char *zoomInIconFile = "/gfx/zoomin.bin";
static const char *zoomOutIconFile = "/gfx/zoomout.bin";

bool isMainScreen = false;
bool isScrolled = true;      
bool isScrollingMap = false;  
bool canScrollMap = false;   
uint8_t activeTile = 0;
uint8_t gpxAction = WPT_NONE;

lv_timer_t *map_inertia_timer = NULL;

extern uint32_t DOUBLE_TOUCH_EVENT;
#ifdef ENABLE_COMPASS
    extern Compass compass;
#endif
extern Gps gps;
extern wayPoint loadWpt;

#ifdef EXTRA_LARGE_SCREEN
    int toolBarOffset = (int)(100 * scaleBut);
    int toolBarSpace  = (int)(60 * scaleBut);
#elif defined(LARGE_SCREEN)
    int toolBarOffset = 100;
    int toolBarSpace  = 60;
#else
    int toolBarOffset = 80;
    int toolBarSpace  = 50;
#endif

lv_obj_t *tilesScreen;
lv_obj_t *compassTile;
lv_obj_t *navTile;
lv_obj_t *mapTile;
lv_obj_t *satTrackTile;
lv_obj_t *nmeaDebugTile;
lv_obj_t *btnZoomIn;
lv_obj_t *btnZoomOut;
lv_obj_t *btnToggle3D;
static lv_obj_t *toggle3DImg;

static lv_obj_t  *btnRec         = nullptr;
static lv_obj_t  *lblRec         = nullptr;
static lv_obj_t  *circleRec      = nullptr;
static lv_obj_t  *recHud         = nullptr;
static lv_timer_t *recTimer      = nullptr;
static lv_obj_t  *summaryOverlay = nullptr;
static bool       recBlinkOn     = false;
lv_obj_t *mapImage;
static lv_image_dsc_t map_img_dsc;
extern Maps mapView;

static constexpr float MAP_INERTIA_FRICTION   = 0.85f; /**< Velocity damping factor applied each inertia tick. */
static constexpr float MAP_INERTIA_VEL_THRESH = 0.1f;  /**< Velocity below which inertia scroll is stopped. */
static constexpr float MAP_HEADING_THRESHOLD  = 2.0f;  /**< Minimum heading change (degrees) that triggers a map redraw. */
static constexpr float MAP_VELOCITY_WEIGHT    = 0.7f;  /**< EMA weight for velocity estimation during drag. */

/**
 * @brief Update compass screen event
 *
 * @details Updates the compass screen UI elements (heading, coordinates, altitude, speed, sunrise/sunset) with current GPS and heading data when the relevant event is triggered.
 *
 * @param event LVGL event pointer.
 */
static void updateCompassScr(lv_observer_t *observer, lv_subject_t *subject)
{
    if (gps.gpsData.sunriseHour[0] == '\0')
        return;
    lv_label_set_text_static(sunriseLabel, gps.gpsData.sunriseHour);
    lv_label_set_text_static(sunsetLabel, gps.gpsData.sunsetHour);
}

/**
 * @brief Show Map Widgets.
 *
 * @details Displays or hides map-related UI widgets based on map user settings 
 */
static void showMapWidgets()
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
static void hideMapWidgets()
{
    lv_obj_add_flag(navArrow, LV_OBJ_FLAG_HIDDEN);  
    lv_obj_add_flag(zoomWidget, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mapSpeed, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(miniCompass, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(scaleWidget, LV_OBJ_FLAG_HIDDEN);
}

static int global_last_heading = -1;

static struct
{
    int16_t lastDispX          = -32768;
    int16_t lastDispY          = -32768;
    int32_t lastRenderedHeading = -1;
    float   lastRenderedLat    = -1.0f;
    float   lastRenderedLon    = -1.0f;
    float   lastManualHeading  = 0.0f;
} mapRenderState;

static struct
{
    int      last_x       = 0;
    int      last_y       = 0;
    uint32_t last_time    = 0;
    bool     dragStarted  = false;
} scrollState;

static volatile bool redrawPending = false;

/**
 * @brief Async callback on the UI thread that requests map composition and displays it.
 *
 * @details On ESP32-S3 requests a tile generate and marks a composition as
 *          pending when the view changed (offset, heading, position, manual
 *          heading or redraw). Once the render task signals MAP_EVENT_DONE,
 *          invalidates the map image and forces a synchronous refresh,
 *          re-arming MAP_EVENT_FREE so the render task can compose the next
 *          frame. On ESP32-P4 composition stays on the GUI thread —
 *          displayMap() is called directly when the view changed or a rendered
 *          frame is ready (MAP_EVENT_DONE), and LVGL draws asynchronously
 *          (original flow, no handshake latency).
 */
static void async_map_update_cb(void * user_data)
{
    __atomic_store_n(&redrawPending, false, __ATOMIC_SEQ_CST);

    if (!isMainScreen || mapImage == NULL || summaryOverlay != nullptr)
        return;

    mapView.requestGenerate(zoom);
    if (mapView.redrawMap && !mapSet.vectorMap)
        xEventGroupSetBits(mapView.mapEventGroup, Maps::MAP_EVENT_DONE);

    const Gps::GpsSnapshot gpsSnap = gps.getSnapshot();
    int32_t currentHeading = lv_subject_get_int(&subject_heading);
    float currentLat = gpsSnap.latitude;
    float currentLon = gpsSnap.longitude;

    bool headingChanged = (abs(currentHeading - mapRenderState.lastRenderedHeading) > MAP_HEADING_THRESHOLD);
    bool positionChanged = (currentLat != mapRenderState.lastRenderedLat || currentLon != mapRenderState.lastRenderedLon);

#if defined(CONFIG_IDF_TARGET_ESP32P4)
    // ESP32-P4 keeps composition on the GUI thread (no handshake latency; the
    // HW rotate fits the GUI budget). displayMap() takes the map mutex and
    // applies its own hysteresis; the MAP_EVENT_DONE condition redraws as soon
    // as the render task has a fresh frame.
    if (mapView.offsetX != mapRenderState.lastDispX ||
        mapView.offsetY != mapRenderState.lastDispY ||
        ((headingChanged || positionChanged) && mapView.followGps) ||
        mapView.redrawMap ||
        (xEventGroupGetBits(mapView.mapEventGroup) & Maps::MAP_EVENT_DONE))
    {
        mapRenderState.lastDispX           = mapView.offsetX;
        mapRenderState.lastDispY           = mapView.offsetY;
        mapRenderState.lastRenderedHeading = currentHeading;
        mapRenderState.lastRenderedLat     = currentLat;
        mapRenderState.lastRenderedLon     = currentLon;
        xEventGroupClearBits(mapView.mapEventGroup, Maps::MAP_EVENT_DONE);
        mapView.mapClimbShift = (climbOverlay != NULL && !lv_obj_has_flag(climbOverlay, LV_OBJ_FLAG_HIDDEN))
                                    ? lv_obj_get_height(climbOverlay) / 2
                                    : 0;
        mapView.displayMap();
        map_img_dsc.data = (const uint8_t *)mapView.mapBuffer;
        lv_obj_invalidate(mapImage);
        mapView.redrawMap = false;

        if (mapView.is3DActive())
        {
            int navOffset = mapView.mapScrHeight / 4;
            if (climbOverlay != NULL && !lv_obj_has_flag(climbOverlay, LV_OBJ_FLAG_HIDDEN))
                navOffset += lv_obj_get_height(climbOverlay) / 2;
            lv_obj_align(navArrow, LV_ALIGN_BOTTOM_MID, 0, -navOffset);
        }
        else
            lv_obj_align(navArrow, LV_ALIGN_CENTER, 0, 0);
    }
#else
    mapView.mapClimbShift = (climbOverlay != NULL && !lv_obj_has_flag(climbOverlay, LV_OBJ_FLAG_HIDDEN))
                                ? lv_obj_get_height(climbOverlay) / 2
                                : 0;

    bool composeNeeded = (mapView.offsetX != mapRenderState.lastDispX ||
                          mapView.offsetY != mapRenderState.lastDispY ||
                          ((headingChanged || positionChanged) && mapView.followGps) ||
                          mapView.manualHeading != mapRenderState.lastManualHeading ||
                          mapView.redrawMap);

    if (composeNeeded)
    {
        mapRenderState.lastDispX           = mapView.offsetX;
        mapRenderState.lastDispY           = mapView.offsetY;
        mapRenderState.lastRenderedHeading = currentHeading;
        mapRenderState.lastRenderedLat     = currentLat;
        mapRenderState.lastRenderedLon     = currentLon;
        mapRenderState.lastManualHeading   = mapView.manualHeading;
        xEventGroupClearBits(mapView.mapEventGroup, Maps::MAP_EVENT_DONE);
        mapView.mapComposePending = true;
        mapView.redrawMap = false;
    }

    if (xEventGroupGetBits(mapView.mapEventGroup) & Maps::MAP_EVENT_DONE)
    {
        xEventGroupClearBits(mapView.mapEventGroup, Maps::MAP_EVENT_DONE);
        mapView.displayMap();
        map_img_dsc.data = (const uint8_t *)mapView.mapBuffer;
        lv_obj_invalidate(mapImage);
        if (mapView.is3DActive())
        {
            int navOffset = mapView.mapScrHeight / 4;
            if (climbOverlay != NULL && !lv_obj_has_flag(climbOverlay, LV_OBJ_FLAG_HIDDEN))
                navOffset += lv_obj_get_height(climbOverlay) / 2;
            lv_obj_align(navArrow, LV_ALIGN_BOTTOM_MID, 0, -navOffset);
        }
        else
            lv_obj_align(navArrow, LV_ALIGN_CENTER, 0, 0);
        lv_refr_now(lv_display_get_default());
        xEventGroupSetBits(mapView.mapEventGroup, Maps::MAP_EVENT_FREE);
    }
#endif

    lv_obj_set_pos(mapImage, 0, 0);
    if (mapSet.showClimb)
        navCtx.climbAnalyzer.updatePosition(currentLat, currentLon, navSet.simNavigation, gps.getSimulationIndex(), navCtx.trackData);

    if (mapSet.showMapSpeed)
        lv_label_set_text_fmt(mapSpeedLabel, "%3d", gpsSnap.speed);
    if (mapSet.showMapScale)
        lv_label_set_text_fmt(scaleLabel, "%s", map_scale[zoom]);
}

/**
 * @brief Thread-safe trigger for map redrawing from background tasks
 */
void triggerMapRedraw()
{
    if (__atomic_exchange_n(&redrawPending, true, __ATOMIC_SEQ_CST))
        return;
    lv_async_call(async_map_update_cb, NULL);
}

/**
 * @brief Force map redraw from non-LVGL context
 *
 * @details Unconditionally resets the redraw guard and queues an
 *          async callback. Use after vTaskResume(guiTaskHandle) to
 *          ensure a fresh render from a stable LVGL task context.
 */
void forceMapRedraw()
{
    __atomic_store_n(&redrawPending, false, __ATOMIC_SEQ_CST);
    lv_async_call(async_map_update_cb, NULL);
}

/**
 * @brief Toggle 3D/2D map view event callback.
 *
 * @param event LVGL event pointer.
 */
static void toggle3DEvent(lv_event_t *event)
{
    int32_t current = lv_subject_get_int(&subject_map_3d);
    lv_subject_set_int(&subject_map_3d, current ? 0 : 1);
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
    if (activeTile != MAP || summaryOverlay != nullptr || lv_subject_get_int(&subject_map_state) != MAP_MODE_FOLLOW)
        return;

    triggerMapRedraw();
}

/**
 * @brief Observer callback for map offset updates (Scroll or Inertia)
 *
 * @details Triggers map redraws automatically when manual offset changes.
 *          Uses lv_async_call to delegate rendering to the UI thread (Core 1).
 *
 * @param observer Pointer to the observer.
 * @param subject Pointer to the subject.
 */
static void map_offset_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    if (activeTile != MAP)
        return;

    triggerMapRedraw();
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
    if (activeTile != MAP || canMoveWidget || summaryOverlay != nullptr || lv_subject_get_int(&subject_map_state) != MAP_MODE_FOLLOW)
        return;

    int32_t newHeading = lv_subject_get_int(subject);
    
    if (abs(newHeading - global_last_heading) > MAP_HEADING_THRESHOLD)
    {
        global_last_heading = newHeading;
        triggerMapRedraw();
    }
}

// Sprite used to render the static elevation profile with LovyanGFX primitives.
// buf: contiguous PSRAM buffer [RGB565 W*2*H bytes | alpha W*H bytes].
// Layout matches LV_COLOR_FORMAT_RGB565A8 expected by lv_draw_buf_init.
static struct
{
    TFT_eSprite   sprite       = TFT_eSprite(&tft);
    uint8_t      *buf          = nullptr;
    lv_draw_buf_t drawBuf      = {};
    bool          profileBuilt = false;
    int           lastSegStart = -1;
    int           lastPosX     = -1;
    int           lastYTop     = -1;
    float         minEle       = 0.0f;
    float         maxEle       = 0.0f;
    float         eleRange     = 1.0f;
} climbState;

/**
 * @brief Build the static elevation profile into climbState.sprite using LovyanGFX primitives.
 *
 * @details Renders the full-track or zoomed profile once. Iterates over canvas columns
 *          (W passes) to find elevation and color per column, then draws a filled
 *          vertical bar with drawFastVLine. O(W) — fast even on ESP32-S3.
 *          After rendering, passes the sprite buffer to lv_canvas_set_buffer so LVGL
 *          displays it without any copy.
 *
 * @param startPt  First navCtx.trackData index of the visible window.
 * @param endPt    Last  navCtx.trackData index of the visible window.
 */
static void buildClimbProfile(int startPt, int endPt)
{
    if (climbCanvas == NULL || navCtx.trackData.size() < 2)
        return;

    int W = lv_obj_get_width(climbCanvas);
    int H = lv_obj_get_height(climbCanvas);
    if (W <= 0 || H <= 0)
        return;

    // Recreate sprite and unified RGB565A8 buffer if size changed
    if (climbState.sprite.width() != W || climbState.sprite.height() != H)
    {
        climbState.sprite.deleteSprite();
        climbState.sprite.setColorDepth(16);
        climbState.sprite.createSprite(W, H);

        if (climbState.buf != nullptr)
            heap_caps_free(climbState.buf);
        // RGB565 (W*2*H) + A8 mask (W*H) contiguous buffer in PSRAM
        climbState.buf = (uint8_t *)heap_caps_malloc(W * 2 * H + W * H, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    if (climbState.buf == nullptr)
        return;

    climbState.sprite.fillScreen(TFT_BLACK);

    float distStart = navCtx.trackData[startPt].accumDist;
    float distEnd   = navCtx.trackData[endPt].accumDist;
    float distRange = distEnd - distStart;
    if (distRange < 1.0f)
        return;

    float minEle = navCtx.trackData[startPt].ele;
    float maxEle = navCtx.trackData[startPt].ele;
    for (int i = startPt + 1; i <= endPt; ++i)
    {
        if (navCtx.trackData[i].ele < minEle)
            minEle = navCtx.trackData[i].ele;
        if (navCtx.trackData[i].ele > maxEle)
            maxEle = navCtx.trackData[i].ele;
    }
    float eleRange = maxEle - minEle;
    if (eleRange < 1.0f)
        eleRange = 1.0f;

    climbState.minEle   = minEle;
    climbState.maxEle   = maxEle;
    climbState.eleRange = eleRange;

    const auto &segs = navCtx.climbAnalyzer.segments();
    auto toCol = [](uint32_t rgb) { return lgfx::rgb888_t((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF); };

    uint8_t *alphaBuf = climbState.buf + W * 2 * H;
    memset(alphaBuf, 0x00, W * H);

    // Linear cursor through navCtx.trackData — O(n) total across all W columns
    int trkCursor = startPt;

    for (int x = 0; x < W; ++x)
    {
        float colDist = distStart + (float)x / (float)(W - 1) * distRange;

        while (trkCursor < endPt - 1 && navCtx.trackData[trkCursor + 1].accumDist < colDist)
            ++trkCursor;

        float d0 = navCtx.trackData[trkCursor].accumDist;
        float d1 = navCtx.trackData[trkCursor + 1].accumDist;
        float e0 = navCtx.trackData[trkCursor].ele;
        float e1 = navCtx.trackData[trkCursor + 1].ele;
        float t  = (d1 > d0) ? (colDist - d0) / (d1 - d0) : 0.0f;
        float ele = e0 + t * (e1 - e0);

        int yTop = calcYTop(ele, minEle, eleRange, H);

        lgfx::rgb888_t col = toCol(0x00C800u);
        for (const auto &seg : segs)
        {
            if (colDist >= navCtx.trackData[seg.startIdx].accumDist &&
                colDist <= navCtx.trackData[seg.endIdx].accumDist)
            {
                // Local grade over a 50m window centred on colDist
                float wHalf = 25.0f;
                float dA = colDist - wHalf;
                float dB = colDist + wHalf;
                if (dA < navCtx.trackData[seg.startIdx].accumDist)
                    dA = navCtx.trackData[seg.startIdx].accumDist;
                if (dB > navCtx.trackData[seg.endIdx].accumDist)
                    dB = navCtx.trackData[seg.endIdx].accumDist;
                int maxIdx = (int)navCtx.trackData.size() - 1;
                int ia = trkCursor;
                while (ia > 0 && navCtx.trackData[ia].accumDist > dA) --ia;
                while (ia < maxIdx && navCtx.trackData[ia + 1].accumDist < dA) ++ia;
                float ta = (ia < maxIdx && navCtx.trackData[ia + 1].accumDist > navCtx.trackData[ia].accumDist)
                           ? (dA - navCtx.trackData[ia].accumDist) / (navCtx.trackData[ia + 1].accumDist - navCtx.trackData[ia].accumDist)
                           : 0.0f;
                float eleA = navCtx.trackData[ia].ele + ta * (ia < maxIdx ? (navCtx.trackData[ia + 1].ele - navCtx.trackData[ia].ele) : 0.0f);
                int ib = ia;
                while (ib < maxIdx && navCtx.trackData[ib + 1].accumDist < dB) ++ib;
                float tb = (ib < maxIdx && navCtx.trackData[ib + 1].accumDist > navCtx.trackData[ib].accumDist)
                           ? (dB - navCtx.trackData[ib].accumDist) / (navCtx.trackData[ib + 1].accumDist - navCtx.trackData[ib].accumDist)
                           : 0.0f;
                float eleB = navCtx.trackData[ib].ele + tb * (ib < maxIdx ? (navCtx.trackData[ib + 1].ele - navCtx.trackData[ib].ele) : 0.0f);
                float winDist = dB - dA;
                float localGrade = (winDist > 1.0f) ? ((eleB - eleA) / winDist * 100.0f) : 0.0f;
                if (localGrade < 0.0f)
                    localGrade = 0.0f;
                col = toCol(climbSegmentColor(localGrade));
                break;
            }
        }

        climbState.sprite.drawFastVLine(x, yTop, H - yTop, col);

        // Alpha mask: opaque only where the bar is drawn
        for (int y = yTop; y < H; ++y)
            alphaBuf[y * W + x] = 0xFF;
    }

    // Copy sprite buffer byte-by-byte swapping pairs: LovyanGFX stores RGB565
    // with bytes already swapped for the ILI9488 bus. RGB565A8 expects unswapped
    // RGB565, so undo the hardware swap here.
    const uint8_t *src = (const uint8_t *)climbState.sprite.getBuffer();
    for (int i = 0; i < W * H; ++i)
    {
        climbState.buf[i * 2]     = src[i * 2 + 1];
        climbState.buf[i * 2 + 1] = src[i * 2];
    }

    uint32_t stride = (uint32_t)W * 2;
    lv_draw_buf_init(&climbState.drawBuf, (uint32_t)W, (uint32_t)H,
                     LV_COLOR_FORMAT_RGB565A8, stride,
                     climbState.buf, stride * (uint32_t)H + (uint32_t)W * (uint32_t)H);
    lv_canvas_set_draw_buf(climbCanvas, &climbState.drawBuf);
    lv_obj_invalidate(climbCanvas);
    climbState.profileBuilt = true;
}

/**
 * @brief Draw a white vertical line + ▼ triangle above the bar to mark GPS position.
 *
 * @details Restores the previous marker area (column + triangle), then draws the white
 *          line within the bar and a ▼ triangle in the TRI_MARGIN zone above yTop.
 *
 * @param posX  Canvas column index for the current GPS position.
 * @param yTop  Top row of the profile bar at posX (already offset by TRI_MARGIN).
 */
static void updateClimbMarker(int posX, int yTop)
{
    if (climbCanvas == NULL || climbState.buf == nullptr)
        return;

    int W = climbState.sprite.width();
    int H = climbState.sprite.height();
    if (W <= 0 || H <= 0)
        return;

    const uint8_t *src   = (const uint8_t *)climbState.sprite.getBuffer();
    uint8_t       *alpha = climbState.buf + W * 2 * H;

    auto restorePixel = [&](int x, int y)
    {
        if (x < 0 || x >= W || y < 0 || y >= H)
            return;
        int i = y * W + x;
        climbState.buf[i * 2]     = src[i * 2 + 1];
        climbState.buf[i * 2 + 1] = src[i * 2];
        uint16_t px = ((uint16_t)src[i * 2 + 1] << 8) | src[i * 2];
        alpha[y * W + x] = (px == 0x0000) ? 0x00 : 0xFF;
    };

    // Restore previous column and triangle
    if (climbState.lastPosX >= 0 && climbState.lastPosX < W)
    {
        for (int y = 0; y < H; ++y)
            restorePixel(climbState.lastPosX, y);

        for (int r = 0; r < TRI_ROWS; ++r)
        {
            int y    = climbState.lastYTop - TRI_GAP - TRI_ROWS + r;
            int half = triMask[r] / 2;
            for (int dx = -half; dx <= half; ++dx)
                restorePixel(climbState.lastPosX + dx, y);
        }
    }

    if (posX >= 0 && posX < W)
    {
        // White line inside the bar
        for (int y = 0; y < H; ++y)
        {
            int i = y * W + posX;
            uint16_t px = ((uint16_t)src[i * 2 + 1] << 8) | src[i * 2];
            if (px != 0x0000)
            {
                climbState.buf[i * 2]     = 0xFF;
                climbState.buf[i * 2 + 1] = 0xFF;
                alpha[y * W + posX] = 0xFF;
            }
        }

        // ▼ triangle in the reserved margin above yTop, with TRI_GAP blank rows
        for (int r = 0; r < TRI_ROWS; ++r)
        {
            int y    = yTop - TRI_GAP - TRI_ROWS + r;
            int half = triMask[r] / 2;
            for (int dx = -half; dx <= half; ++dx)
            {
                int tx = posX + dx;
                if (tx < 0 || tx >= W || y < 0 || y >= H)
                    continue;
                int i = y * W + tx;
                climbState.buf[i * 2]     = 0xFF;
                climbState.buf[i * 2 + 1] = 0xFF;
                alpha[y * W + tx]   = 0xFF;
            }
        }

        climbState.lastPosX = posX;
        climbState.lastYTop = yTop;
    }

    lv_obj_invalidate(climbCanvas);
}

/**
 * @brief Observer callback for subject_climb_active — controls overlay visibility only.
 */
static void climb_active_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    if (climbOverlay == NULL)
        return;

    if (lv_subject_get_int(&subject_climb_active) == 0)
    {
        lv_obj_add_flag(climbOverlay, LV_OBJ_FLAG_HIDDEN);
        climbState.profileBuilt = false;
        climbState.lastSegStart = -1;
        climbState.lastPosX     = -1;
        climbState.lastYTop     = -1;
    }
    else
        lv_obj_clear_flag(climbOverlay, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief Observer callback for subject_climb_idx — updates labels, profile and marker.
 *
 * @details Only called when active==1. Finds the active segment using the same
 *          anticipation window as updatePosition() so buildClimbProfile is invoked
 *          from the first frame the overlay appears, not only once inside the climb.
 */
static void climb_idx_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    if (climbOverlay == NULL)
        return;
    if (lv_subject_get_int(&subject_climb_active) == 0)
        return;

    int32_t dist      = lv_subject_get_int(&subject_climb_dist);
    int32_t gain      = lv_subject_get_int(&subject_climb_gain);
    int32_t grade10   = lv_subject_get_int(&subject_climb_grade);
    float   grade     = grade10 / 10.0f;
    int32_t activeIdx = lv_subject_get_int(&subject_climb_idx);

    int32_t seg_num   = lv_subject_get_int(&subject_climb_seg);
    int32_t seg_total = lv_subject_get_int(&subject_climb_total);
    int32_t cat       = lv_subject_get_int(&subject_climb_cat);
    int32_t avg10     = lv_subject_get_int(&subject_climb_avg_grade);
    int32_t tot_dist  = lv_subject_get_int(&subject_climb_total_dist);
    int32_t tot_gain  = lv_subject_get_int(&subject_climb_total_gain);
    int32_t appr      = lv_subject_get_int(&subject_climb_approaching);

    lv_label_set_text_fmt(climbTotalDistLabel, "%.1fkm", tot_dist / 1000.0f);
    lv_label_set_text_fmt(climbTotalGainLabel, "%dm D+", tot_gain);
    lv_label_set_text_fmt(climbAvgGradeLabel,  "avg %.1f%%", avg10 / 10.0f);
    lv_label_set_text_fmt(climbSegLabel, "%d/%d", seg_num, seg_total);

    static const char *catStr[] = { "", "HC", "CAT 1", "CAT 2", "CAT 3", "CAT 4" };
    lv_label_set_text_static(climbCatLabel, (cat >= 0 && cat <= 5) ? catStr[cat] : "");

    if (appr)
        lv_label_set_text_fmt(climbDistLabel, LV_SYMBOL_RIGHT " %dm", dist);
    else
        lv_label_set_text_fmt(climbDistLabel, LV_SYMBOL_UP " %dm", dist);
    lv_label_set_text_fmt(climbGainLabel, "%dm D+", gain);
    lv_label_set_text_fmt(climbGradeLabel, "%.1f%%", grade);

    int W = lv_obj_get_width(climbCanvas);
    if (W <= 0)
        return;

    // Same anticipation condition as updatePosition() — covers pre-climb phase too
    const std::vector<ClimbSegment>& segs = navCtx.climbAnalyzer.segments();
    float curDistObs = navCtx.trackData[(int)activeIdx].accumDist;
    const ClimbSegment* seg = nullptr;
    for (const ClimbSegment& s : segs)
    {
        float segStartDist = navCtx.trackData[s.startIdx].accumDist;
        float segEndDist   = navCtx.trackData[s.endIdx].accumDist;
        if (curDistObs <= segEndDist && segStartDist - curDistObs <= CLIMB_ANTICIPATION_M)
        {
            seg = &s;
            break;
        }
    }
    if (seg == nullptr)
        return;

    float preStartDist = navCtx.trackData[seg->startIdx].accumDist - CLIMB_ANTICIPATION_M;
    int startPt = seg->startIdx;
    while (startPt > 0 && navCtx.trackData[startPt - 1].accumDist >= preStartDist)
        --startPt;

    int endPt  = seg->endIdx;
    float dRange = navCtx.trackData[endPt].accumDist - navCtx.trackData[startPt].accumDist;
    float dPos   = curDistObs - navCtx.trackData[startPt].accumDist;
    int posX = (dRange > 0.0f) ? (int)(dPos / dRange * (W - 1)) : 0;
    if (posX < 0)
        posX = 0;
    if (posX >= W)
        posX = W - 1;

    if (!climbState.profileBuilt || seg->startIdx != climbState.lastSegStart)
    {
        climbState.lastPosX = -1;
        climbState.lastYTop = -1;
        buildClimbProfile(startPt, endPt);
        climbState.lastSegStart = seg->startIdx;
    }

    int H = lv_obj_get_height(climbCanvas);
    float curEleObs = navCtx.trackData[(int)activeIdx].ele;
    int yTop = calcYTop(curEleObs, climbState.minEle, climbState.eleRange, H);

    updateClimbMarker(posX, yTop);
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
 * @brief Observer callback for subject_map_3d — toggles pseudo-3D perspective in real time.
 *
 * @details Synchronizes mapSet.map3D with the subject value, invalidates the tile cache,
 *          and schedules an immediate redraw via lv_async_call.
 *
 * @param observer Pointer to the observer.
 * @param subject Pointer to the subject.
 */
static void map_3d_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    mapSet.map3D = (lv_subject_get_int(subject) != 0);
    if (toggle3DImg != NULL)
        lv_img_set_src(toggle3DImg, mapSet.map3D ? toggle3DIconFile : toggle2DIconFile);
    mapView.updateMap();
    triggerMapRedraw();
}

/**
 * @brief Get active tile
 *
 * @details Handles tileview scroll event, updates active tile index, and manages map/widget visibility and bar status.
 *
 * @param event LVGL event pointer.
 */
static void getActTile(lv_event_t *event)
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
        closeOptionsPanel();
    lv_obj_t *actTile = lv_tileview_get_tile_act(tilesScreen);
    if (actTile == NULL)
        return;
    activeTile = lv_obj_get_x(actTile) / TFT_WIDTH;
    if (activeTile == NAV && navTile != NULL)
        lv_obj_send_event(navTile, LV_EVENT_VALUE_CHANGED, NULL);
}

/**
 * @brief Tile start scrolling event
 *
 * @details Handles the beginning of a tile scroll event by resetting scroll and map redraw flags and deleting map screen sprites.
 *
 * @param event LVGL event pointer.
 */
static void scrollTile(lv_event_t *event)
{
    isScrolled = false;
    mapView.redrawMap = false;
    mapView.deleteMapScrSprites();
}

/**
 * @brief Update map event
 *
 * @details Handles map update events by generating and displaying the map (vector or render), updating map speed, scale, and compass widgets according to current settings.
 *
 * @param event LVGL event pointer.
 */
static void updateMap(lv_event_t *event)
{
    triggerMapRedraw();
}

/**
 * @brief Shows or hides the zoom in/out buttons.
 *
 * @param show true to make buttons visible and interactive, false to hide them.
 */
static void setZoomButtonsVisible(bool show)
{
    if (show)
    {
        lv_obj_add_flag(btnZoomOut, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
        lv_obj_add_flag(btnZoomIn, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
        lv_obj_clear_flag(btnZoomOut, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(btnZoomIn, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_clear_flag(btnZoomOut, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
        lv_obj_clear_flag(btnZoomIn, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
        lv_obj_add_flag(btnZoomOut, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(btnZoomIn, LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * @brief Map Tool Bar Event.
 *
 * @details Handles map toolbar visibility toggling, zoom button states, map scrollability, and map centering on GPS.
 *
 * @param event LVGL event pointer.
 */
static void mapToolBarEvent(lv_event_t *event)
{
    showMapToolBar = !showMapToolBar;
    canScrollMap = !canScrollMap;
    isScrollingMap = false;
    if (!showMapToolBar)
    {
        setZoomButtonsVisible(false);
        lv_obj_add_flag(tilesScreen, LV_OBJ_FLAG_SCROLLABLE);
        const Gps::GpsSnapshot gpsSnap = gps.getSnapshot();
        mapView.centerOnGps(gpsSnap.latitude, gpsSnap.longitude);
        lv_subject_set_int(&subject_map_state, MAP_MODE_FOLLOW);
        mapView.updateMap();
        lv_obj_clear_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
        lv_subject_set_int(&subject_map_offset_x, mapView.offsetX);
        lv_subject_set_int(&subject_map_offset_y, mapView.offsetY);
        triggerMapRedraw();
    }
    else
    {
        setZoomButtonsVisible(true);
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
static void map_inertia_timer_cb(lv_timer_t * t)
{
    float dt = 20.0f; // Fixed period defined in createMainScr()
    if (mapView.velocityX != 0 || mapView.velocityY != 0)
    {
        float dx = mapView.velocityX * dt;
        float dy = mapView.velocityY * dt;
        mapView.scrollMap((int16_t)dx, (int16_t)dy);

        bool renderBusy = mapView.isRendering() || mapView.isScrollDeferred();
        float currentFriction = renderBusy ? MAP_INERTIA_FRICTION : mapView.friction;
        mapView.velocityX *= currentFriction;
        mapView.velocityY *= currentFriction;

        if (abs(mapView.velocityX) < MAP_INERTIA_VEL_THRESH)
            mapView.velocityX = 0;
        if (abs(mapView.velocityY) < MAP_INERTIA_VEL_THRESH)
            mapView.velocityY = 0;

        lv_subject_set_int(&subject_map_offset_x, mapView.offsetX);
        lv_subject_set_int(&subject_map_offset_y, mapView.offsetY);
    }
    else
    {
        lv_timer_pause(t);
        mapView.setInertia(false);
        mapView.commitScroll();
        triggerMapRedraw();
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
static void scrollMapEvent(lv_event_t *event)
{
    extern volatile bool twoFingerGesture;
    if (canScrollMap)
    {
        lv_event_code_t code = lv_event_get_code(event);
        if (twoFingerGesture && code != LV_EVENT_RELEASED && code != LV_EVENT_PRESS_LOST)
            return;
        lv_indev_t * indev = lv_event_get_indev(event);
        lv_point_t p;

        switch (code)
        {
            case LV_EVENT_PRESSED:
                lv_indev_get_point(indev, &p);
                scrollState.last_x      = p.x;
                scrollState.last_y      = p.y;
                scrollState.last_time   = millis_idf();
                scrollState.dragStarted = false;
                isScrollingMap = true;
                mapView.velocityX = 0;
                mapView.velocityY = 0;
                mapView.setInertia(false);
                lv_subject_set_int(&subject_map_state, MAP_MODE_MANUAL);
                if (map_inertia_timer != NULL)
                    lv_timer_pause(map_inertia_timer);
                break;

            case LV_EVENT_PRESSING:
            {
                lv_indev_get_point(indev, &p);
                uint32_t current_time = millis_idf();
                int dx = p.x - scrollState.last_x;
                int dy = p.y - scrollState.last_y;
                uint32_t dt = current_time - scrollState.last_time;

                if (!scrollState.dragStarted)
                {
                    const int START_THRESHOLD = 12;
                    if (abs(dx) > START_THRESHOLD || abs(dy) > START_THRESHOLD)
                    {
                        scrollState.dragStarted = true;
                        lv_obj_add_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
                    }
                }

                if (scrollState.dragStarted && dt > 0)
                {
                    mapView.scrollMap(-dx, -dy);
                    mapView.velocityX = mapView.velocityX * (1.0f - MAP_VELOCITY_WEIGHT) + (-(float)dx / (float)dt) * MAP_VELOCITY_WEIGHT;
                    mapView.velocityY = mapView.velocityY * (1.0f - MAP_VELOCITY_WEIGHT) + (-(float)dy / (float)dt) * MAP_VELOCITY_WEIGHT;
                    scrollState.last_x    = p.x;
                    scrollState.last_y    = p.y;
                    scrollState.last_time = current_time;
                    lv_subject_set_int(&subject_map_offset_x, mapView.offsetX);
                    lv_subject_set_int(&subject_map_offset_y, mapView.offsetY);
                }
                break;
            }
            case LV_EVENT_RELEASED:
            case LV_EVENT_PRESS_LOST:
                lv_obj_clear_flag(navArrow, LV_OBJ_FLAG_HIDDEN);
                isScrollingMap = false;
                scrollState.dragStarted = false;
                if (abs(mapView.velocityX) > MAP_INERTIA_VEL_THRESH || abs(mapView.velocityY) > MAP_INERTIA_VEL_THRESH)
                {
                    lv_subject_set_int(&subject_map_state, MAP_MODE_INERTIA);
                    mapView.setInertia(true);
                    if (map_inertia_timer != NULL)
                        lv_timer_resume(map_inertia_timer);
                }
                else
                {
                    mapView.velocityX = 0;
                    mapView.velocityY = 0;
                    mapView.commitScroll();
                    triggerMapRedraw();
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
static void zoomEvent(lv_event_t *event)
{
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(event);
    if ( obj == btnZoomIn && ( zoom >= minZoom && zoom < maxZoom ) )
        zoom++;
    else if ( obj == btnZoomOut && ( zoom <= maxZoom && zoom > minZoom ) )
        zoom--;
    
    mapView.updateMap();
    lv_subject_set_int(&subject_map_offset_x, mapView.offsetX);
    lv_subject_set_int(&subject_map_offset_y, mapView.offsetY);
    triggerMapRedraw();
    lv_label_set_text_fmt(zoomLabel, "%2d", zoom);
}

/**
 * @brief Handles navigation waypoint screen updates 
 *
 * @param event LVGL event pointer.
 */
static void updateNavEvent(lv_event_t *event)
{
    const Gps::GpsSnapshot gpsSnap = gps.getSnapshot();
    int wptDistance = (int)calcDist(gpsSnap.latitude, gpsSnap.longitude, loadWpt.lat, loadWpt.lon);
    lv_label_set_text_fmt(distNav, "%d m.", wptDistance);
    if (wptDistance <= 30)
    {
        LV_IMG_DECLARE(navfinish);
        lv_img_set_src(arrowNav, &navfinish);
        lv_img_set_angle(arrowNav, 0);
    }
    else
    {
        float navHeading = (float)lv_subject_get_int(&subject_heading);
        float wptCourse = calcCourse(gpsSnap.latitude, gpsSnap.longitude, loadWpt.lat, loadWpt.lon) - navHeading;
        lv_img_set_angle(arrowNav, (wptCourse * 10));
    }
}

/**
 * @brief Create Image for Map.
 *
 * @details Initializes and creates the image object for rendering the map on the specified screen.
 *
 * @param screen Pointer to the LVGL screen object.
 */
static void createMapImage(_lv_obj_t *screen)
{
    mapImage = lv_image_create(screen);
    lv_obj_set_scrollbar_mode(mapImage, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(mapImage, LV_OBJ_FLAG_FLOATING);

    // Initialize Image Descriptor for LVGL 9
    map_img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    map_img_dsc.header.cf = LV_COLOR_FORMAT_RGB565_SWAPPED;
    map_img_dsc.header.flags = 0;
    map_img_dsc.header.w = mapView.mapScrWidth;
    map_img_dsc.header.h = mapView.mapScrHeight;
    map_img_dsc.header.stride = mapView.mapScrWidth * 2;
    map_img_dsc.data_size = mapView.mapScrWidth * mapView.mapScrHeight * 2;
    map_img_dsc.data = (const uint8_t *)mapView.mapBuffer;

    lv_image_set_src(mapImage, &map_img_dsc);
}

/**
 * @brief Clear the overlay pointer once LVGL has actually destroyed it.
 *
 * @param e LVGL event (LV_EVENT_DELETE).
 */
static void summaryDeleteEvent(lv_event_t *e)
{
    summaryOverlay = nullptr;
}

/**
 * @brief Close the summary overlay when the user taps OK.
 *
 * @param e LVGL event (LV_EVENT_CLICKED).
 */
static void summaryOkEvent(lv_event_t *e)
{
    if (summaryOverlay == nullptr || !lv_obj_is_valid(summaryOverlay))
        return;

    if (lv_obj_has_flag(summaryOverlay, LV_OBJ_FLAG_HIDDEN))
        return;

    lv_obj_add_flag(summaryOverlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_delete_async(summaryOverlay);
}

/**
 * @brief Display the post-recording summary overlay.
 *
 * @details Shows distance, total time, moving time, speeds, elevation and
 *          the GPX filename. Must be called while lvgl_mutex is held.
 */
static void showLoggerSummary()
{
    if (summaryOverlay != nullptr)
        return;

    const LoggerStats& s = gpxLogger.stats();

    summaryOverlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(summaryOverlay, TFT_WIDTH, TFT_HEIGHT);
    lv_obj_set_pos(summaryOverlay, 0, 0);
    lv_obj_add_flag(summaryOverlay, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_style_bg_color(summaryOverlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(summaryOverlay, LV_OPA_70, 0);
    lv_obj_set_style_border_width(summaryOverlay, 0, 0);
    lv_obj_clear_flag(summaryOverlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(summaryOverlay, summaryDeleteEvent, LV_EVENT_DELETE, nullptr);

    lv_obj_t *card = lv_obj_create(summaryOverlay);
    lv_obj_set_size(card, TFT_WIDTH - 20, TFT_HEIGHT - 50);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_make(25, 25, 25), 0);
#if defined(EXTRA_LARGE_SCREEN) || defined(T4_S3)
    lv_obj_set_style_pad_all(card, (int)(10 * scale), 0);
#else
    lv_obj_set_style_pad_all(card, 10, 0);
#endif
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, LV_SYMBOL_OK " Track saved");
#if defined(EXTRA_LARGE_SCREEN) || defined(T4_S3)
    lv_obj_set_style_text_font(title, fontLarge, 0);
#elif defined(LARGE_SCREEN)
    lv_obj_set_style_text_font(title, fontLarge, 0);
#else
    lv_obj_set_style_text_font(title, fontMedium, 0);
#endif
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    char buf[128];
    char vbuf[24];
    int  y;
#if defined(EXTRA_LARGE_SCREEN) || defined(T4_S3)
    y = (int)(35 * scale);
#else
    y = 30;
#endif

    auto addRow = [&](const char *label, const char *val)
    {
        lv_obj_t *lbl = lv_label_create(card);
        snprintf(buf, sizeof(buf), "%s  %s", label, val);
        lv_label_set_text(lbl, buf);
#if defined(EXTRA_LARGE_SCREEN) || defined(T4_S3)
        lv_obj_set_style_text_font(lbl, fontDefault, 0);
#elif defined(LARGE_SCREEN)
        lv_obj_set_style_text_font(lbl, fontDefault, 0);
#else
        lv_obj_set_style_text_font(lbl, fontSmall, 0);
#endif
        lv_obj_set_pos(lbl, 0, y);
#if defined(EXTRA_LARGE_SCREEN) || defined(T4_S3)
        y += (int)(20 * scale);
#else
        y += 18;
#endif
    };

    if (s.totalDistM >= 1000.0f)
    {
        dtostrf(s.totalDistM / 1000.0f, 5, 2, vbuf);
        const char *p = vbuf; while (*p == ' ') p++;
        char tmp[24]; snprintf(tmp, sizeof(tmp), "%s km", p);
        addRow("Distance:", tmp);
    }
    else
    {
        snprintf(vbuf, sizeof(vbuf), "%d m", (int)s.totalDistM);
        addRow("Distance:", vbuf);
    }

    snprintf(vbuf, sizeof(vbuf), "%02u:%02u", s.totalTimeSec / 60, s.totalTimeSec % 60);
    addRow("Total time:", vbuf);

    snprintf(vbuf, sizeof(vbuf), "%02u:%02u", s.movingTimeSec / 60, s.movingTimeSec % 60);
    addRow("Moving time:", vbuf);

    { char tmp[24]; dtostrf(s.maxSpeedKmh, 4, 1, tmp);
      const char *p = tmp; while (*p == ' ') p++;
      snprintf(vbuf, sizeof(vbuf), "%s km/h", p); }
    addRow("Max speed:", vbuf);

    { char tmp[24]; dtostrf(s.avgSpeedKmh, 4, 1, tmp);
      const char *p = tmp; while (*p == ' ') p++;
      snprintf(vbuf, sizeof(vbuf), "%s km/h", p); }
    addRow("Avg speed:", vbuf);

    snprintf(vbuf, sizeof(vbuf), "+%dm / -%dm", (int)s.gainPos, (int)s.gainNeg);
    addRow("Elevation:", vbuf);

    snprintf(vbuf, sizeof(vbuf), "%lu pts", (unsigned long)s.numPoints);
    addRow("Points:", vbuf);

    const char *fn    = s.filename;
    const char *slash = strrchr(fn, '/');
    addRow("File:", slash ? slash + 1 : fn);

    lv_obj_t *btnOk = lv_btn_create(card);
#if defined(EXTRA_LARGE_SCREEN) || defined(T4_S3)
    lv_obj_set_size(btnOk, (int)(80 * scaleBut), (int)(35 * scaleBut));
#else
    lv_obj_set_size(btnOk, 80, 35);
#endif
    lv_obj_align(btnOk, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_t *lblOk = lv_label_create(btnOk);
    lv_label_set_text(lblOk, "OK");
#if defined(EXTRA_LARGE_SCREEN) || defined(T4_S3)
    lv_obj_set_style_text_font(lblOk, fontDefault, 0);
#endif
    lv_obj_center(lblOk);
    lv_obj_add_event_cb(btnOk, summaryOkEvent, LV_EVENT_CLICKED, nullptr);
}

/**
 * @brief Handle REC button click: toggle recording and update UI labels.
 *
 * @param e LVGL event (LV_EVENT_CLICKED).
 */
static void recBtnEvent(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED)
        return;

    LoggerState st = gpxLogger.state();
    if (st == LoggerState::IDLE)
    {
        gpxLogger.start();
        lv_obj_add_flag(circleRec, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(lblRec, LV_SYMBOL_STOP);
        lv_obj_clear_flag(lblRec, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(btnRec, lv_color_make(200, 0, 0), 0);
    }
    else
    {
        gpxLogger.stop();
        lv_obj_clear_flag(circleRec, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(lblRec, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(btnRec, lv_color_make(50, 50, 50), 0);
        showLoggerSummary();
    }
}

/**
 * @brief 500 ms timer: blink the REC button and refresh the HUD label.
 *
 * @param t LVGL timer handle.
 */
static void recTimerCb(lv_timer_t *t)
{
    if (btnRec == nullptr)
        return;

    LoggerState st = gpxLogger.state();

    if (!storage.getSdLoaded() && st == LoggerState::IDLE)
    {
        lv_obj_add_flag(btnRec, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    recBlinkOn = !recBlinkOn;

    // Hide button when following a route or navigating to a waypoint
    bool navActive = isTrackLoaded || mapView.getHasWaypoint();
    if (navActive && st == LoggerState::IDLE)
    {
        lv_obj_add_flag(btnRec, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_clear_flag(btnRec, LV_OBJ_FLAG_HIDDEN);

    if (st == LoggerState::IDLE)
    {
        lv_obj_clear_flag(circleRec, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(lblRec, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(btnRec, lv_color_make(50, 50, 50), 0);
    }
    else if (st == LoggerState::RECORDING)
    {
        lv_obj_add_flag(circleRec, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(lblRec, LV_SYMBOL_STOP);
        lv_obj_clear_flag(lblRec, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(btnRec,
            recBlinkOn ? lv_color_make(200, 0, 0) : lv_color_make(80, 0, 0), 0);
    }
    else
    {
        lv_obj_add_flag(circleRec, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(lblRec, LV_SYMBOL_STOP);
        lv_obj_clear_flag(lblRec, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(btnRec, lv_color_make(200, 120, 0), 0);
    }

    if (recBlinkOn && recHud != nullptr)
    {
        if (st != LoggerState::IDLE)
        {
            lv_obj_clear_flag(recHud, LV_OBJ_FLAG_HIDDEN);
            float    dist   = gpxLogger.stats().totalDistM;
            int32_t  gain   = gpxLogger.stats().gainPos;
            float    grade  = gpxLogger.currentGrade();
            uint32_t movMs  = gpxLogger.movingElapsedMs();
            uint32_t movMin = (movMs / 1000) / 60;
            uint32_t movSec = (movMs / 1000) % 60;
            const char *arrow = (grade > 0.5f) ? LV_SYMBOL_UP : (grade < -0.5f) ? LV_SYMBOL_DOWN : "";
            char gradeBuf[8];
            dtostrf(grade < 0.0f ? -grade : grade, 4, 1, gradeBuf);
            const char *g = gradeBuf; while (*g == ' ') g++;
            char buf[80];
            if (dist >= 1000.0f)
            {
                char dbuf[12]; dtostrf(dist / 1000.0f, 5, 1, dbuf);
                const char *p = dbuf; while (*p == ' ') p++;
                snprintf(buf, sizeof(buf), LV_SYMBOL_GPS " %skm  %02lu:%02lu\n" LV_SYMBOL_UP " %ldm  %s%s%%",
                    p, (unsigned long)movMin, (unsigned long)movSec, (long)gain, arrow, g);
            }
            else
                snprintf(buf, sizeof(buf), LV_SYMBOL_GPS " %dm  %02lu:%02lu\n" LV_SYMBOL_UP " %ldm  %s%s%%",
                    (int)dist, (unsigned long)movMin, (unsigned long)movSec, (long)gain, arrow, g);
            lv_label_set_text(recHud, buf);
        }
        else
            lv_obj_add_flag(recHud, LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * @brief Create Main Screen.
 *
 * @details Initializes and configures the main screen and its tiles, widgets, and event callbacks 
 */
void createMainScr()
{
    mainScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(mainScreen, LV_OBJ_FLAG_SCROLLABLE);
    tilesScreen = lv_tileview_create(mainScreen);
    compassTile = lv_tileview_add_tile(tilesScreen, 0, 0, LV_DIR_RIGHT);
    mapTile = lv_tileview_add_tile(tilesScreen, 1, 0, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_RIGHT));
    navTile = lv_tileview_add_tile(tilesScreen, 2, 0, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_RIGHT));
    lv_obj_add_flag(navTile, LV_OBJ_FLAG_HIDDEN);
    // The satellite tile keeps a right swipe only when the NMEA debug tile exists.
    lv_dir_t satTileDir = nmeaDebugTileEnabled ? (lv_dir_t)(LV_DIR_LEFT | LV_DIR_RIGHT) : LV_DIR_LEFT;
    satTrackTile = lv_tileview_add_tile(tilesScreen, 3, 0, satTileDir);
    if (nmeaDebugTileEnabled)
        nmeaDebugTile = lv_tileview_add_tile(tilesScreen, 4, 0, LV_DIR_LEFT);
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
    // mainScreen never destroyed — manual observer intentional (updateCompassScr targets two labels, no single obj)
    lv_subject_add_observer(&subject_sunrise, updateCompassScr, NULL);
    createMapImage(mapTile);
    navArrowWidget(mapTile);
    mapZoomWidget(mapTile);
    mapSpeedWidget(mapTile);
    mapCompassWidget(mapTile);
    mapScaleWidget(mapTile);
    turnByTurnWidget(mapTile);
    climbWidget(mapTile);
    lv_subject_add_observer_obj(&subject_climb_active, climb_active_observer_cb, climbOverlay, NULL);
    lv_subject_add_observer_obj(&subject_climb_idx,    climb_idx_observer_cb,    climbOverlay, NULL);
    lv_subject_add_observer_obj(&subject_heading, map_heading_observer_cb, mapTile, NULL);
    lv_subject_add_observer_obj(&subject_lat, map_position_observer_cb, mapTile, NULL);
    lv_subject_add_observer_obj(&subject_lon, map_position_observer_cb, mapTile, NULL);
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
    lv_subject_add_observer_obj(&subject_map_offset_x, map_offset_observer_cb, mapTile, NULL);
    lv_subject_add_observer_obj(&subject_map_offset_y, map_offset_observer_cb, mapTile, NULL);
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
    btnToggle3D = lv_obj_create(mapTile);
#if defined(EXTRA_LARGE_SCREEN) || defined(T4_S3)
    lv_obj_set_size(btnToggle3D, (int)(60 * scaleBut), (int)(60 * scaleBut));
#else
    lv_obj_set_size(btnToggle3D, 60, 60);
#endif
    lv_obj_clear_flag(btnToggle3D, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(btnToggle3D, &styleMapWidget, 0);
    lv_obj_add_flag(btnToggle3D, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_FLOATING));
#if defined(EXTRA_LARGE_SCREEN) || defined(T4_S3)
    lv_obj_align(btnToggle3D, LV_ALIGN_TOP_RIGHT, 0, (int)(170 * scale));
#else
    lv_obj_align(btnToggle3D, LV_ALIGN_TOP_RIGHT, 0, 170);
#endif
    toggle3DImg = lv_img_create(btnToggle3D);
    lv_img_set_zoom(toggle3DImg, buttonScale);
    lv_obj_center(toggle3DImg);
    lv_obj_add_flag(btnToggle3D, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(btnToggle3D, toggle3DEvent, LV_EVENT_CLICKED, NULL);
    lv_subject_set_int(&subject_map_3d, mapSet.map3D ? 1 : 0);
    lv_subject_add_observer_obj(&subject_map_3d, map_3d_observer_cb, mapTile, NULL);
    satelliteScr(satTrackTile);
    if (nmeaDebugTileEnabled)
        nmeaDebugScr(nmeaDebugTile);
    // timer is permanent — mainScreen is never destroyed
    map_inertia_timer = lv_timer_create(map_inertia_timer_cb, 20, NULL);
    lv_timer_pause(map_inertia_timer);

    // ── GPX Logger REC button ─────────────────────────────────────────────
    if (storage.getSdLoaded())
    {
        btnRec = lv_obj_create(mapTile);
    #if defined(EXTRA_LARGE_SCREEN) || defined(T4_S3)
        lv_obj_set_size(btnRec, (int)(50 * scaleBut), (int)(50 * scaleBut));
    #else
        lv_obj_set_size(btnRec, 50, 50);
    #endif
        lv_obj_clear_flag(btnRec, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_style(btnRec, &styleMapWidget, 0);
        lv_obj_add_flag(btnRec, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_FLOATING));
    #if defined(EXTRA_LARGE_SCREEN) || defined(T4_S3)
        lv_obj_align_to(btnRec, zoomWidget, LV_ALIGN_OUT_BOTTOM_MID, 0, (int)(5 * scaleBut));
    #else
        lv_obj_align_to(btnRec, zoomWidget, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    #endif
        lv_obj_set_style_bg_color(btnRec, lv_color_make(50, 50, 50), 0);
        circleRec = lv_obj_create(btnRec);
    #if defined(EXTRA_LARGE_SCREEN) || defined(T4_S3)
        lv_obj_set_size(circleRec, (int)(16 * scaleBut), (int)(16 * scaleBut));
    #else
        lv_obj_set_size(circleRec, 16, 16);
    #endif
        lv_obj_set_style_radius(circleRec, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(circleRec, lv_color_make(200, 0, 0), 0);
        lv_obj_set_style_bg_opa(circleRec, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(circleRec, 0, 0);
        lv_obj_clear_flag(circleRec, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
        lv_obj_center(circleRec);
        lblRec = lv_label_create(btnRec);
        lv_label_set_text(lblRec, LV_SYMBOL_STOP);
        lv_obj_set_style_text_font(lblRec, fontLarge, 0);
        lv_obj_center(lblRec);
        lv_obj_add_flag(lblRec, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_event_cb(btnRec, recBtnEvent, LV_EVENT_CLICKED, nullptr);

        recHud = lv_label_create(mapTile);
        lv_label_set_text(recHud, "");
        lv_obj_add_style(recHud, &styleMapWidget, 0);
        lv_obj_set_style_text_font(recHud, fontMedium, 0);
        lv_obj_set_style_text_color(recHud, lv_color_white(), 0);
        lv_obj_add_flag(recHud, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_HIDDEN));
    #if defined(EXTRA_LARGE_SCREEN) || defined(T4_S3)
        lv_obj_align_to(recHud, mapSpeed, LV_ALIGN_OUT_TOP_LEFT, 0, (int)(-25 * scale));
    #else
        lv_obj_align_to(recHud, mapSpeed, LV_ALIGN_OUT_TOP_LEFT, 0, -25);
    #endif

        recTimer = lv_timer_create(recTimerCb, 500, nullptr);

        gpxLogger.init();
    }

    #ifdef BOARD_HAS_PSRAM
        #ifdef TDECK_ESP32S3
            createSatRadar(constMsg);
            lv_obj_align(satRadar, LV_ALIGN_CENTER, 0, 0);
        #endif
    #endif
    if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        lv_subject_set_int(&subject_lat, (int32_t)(gps.getLat() * 1000000.0f));
        lv_subject_set_int(&subject_lon, (int32_t)(gps.getLon() * 1000000.0f));
        xSemaphoreGive(lvgl_mutex);
    }
}
