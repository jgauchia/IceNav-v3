/**
 * @file mainScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Main Screen
 * @version 0.2.9
 * @date 2026-06
 */

#include "mainScr.hpp"
#include "tasks.hpp"
#include "lv_subjects.hpp"
#include "climbAnalyzer.hpp"

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
extern TrackVector trackData;
extern ClimbAnalyzer climbAnalyzer;

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
lv_obj_t *nmeaDebugTile;
lv_obj_t *btnZoomIn;
lv_obj_t *btnZoomOut;
lv_obj_t *btnToggle3D;
static lv_obj_t *toggle3DImg;
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
} mapRenderState;

static struct
{
    int      last_x       = 0;
    int      last_y       = 0;
    uint32_t last_time    = 0;
    bool     dragStarted  = false;
} scrollState;

/**
 * @brief Async callback to delegate map redrawing to UI thread (Core 1)
 */
static void async_map_update_cb(void * user_data)
{
    if (!isMainScreen || mapImage == NULL)
        return;

    mapView.generateMap(zoom);
    if (mapView.redrawMap && !mapSet.vectorMap)
        xEventGroupSetBits(mapView.mapEventGroup, Maps::MAP_EVENT_DONE);

    int32_t currentHeading = lv_subject_get_int(&subject_heading);
    float currentLat = gps.gpsData.latitude;
    float currentLon = gps.gpsData.longitude;

    bool headingChanged = (abs(currentHeading - mapRenderState.lastRenderedHeading) > MAP_HEADING_THRESHOLD);
    bool positionChanged = (currentLat != mapRenderState.lastRenderedLat || currentLon != mapRenderState.lastRenderedLon);

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
        mapView.displayMap();
        map_img_dsc.data = (const uint8_t *)mapView.mapBuffer;
        lv_obj_invalidate(mapImage);
        mapView.redrawMap = false;

        if (mapView.is3DActive())
            lv_obj_align(navArrow, LV_ALIGN_BOTTOM_MID, 0, -(mapView.mapScrHeight / 4));
        else
            lv_obj_align(navArrow, LV_ALIGN_CENTER, 0, 0);
    }

    lv_obj_set_pos(mapImage, 0, 0);
    if (mapSet.showClimb)
        climbAnalyzer.updatePosition(gps.gpsData.latitude, gps.gpsData.longitude, navSet.simNavigation, gps.getSimulationIndex(), trackData);

    if (mapSet.showMapSpeed)
        lv_label_set_text_fmt(mapSpeedLabel, "%3d", gps.gpsData.speed);
    if (mapSet.showMapScale)
        lv_label_set_text_fmt(scaleLabel, "%s", map_scale[zoom]);
}

/**
 * @brief Thread-safe trigger for map redrawing from background tasks
 */
void triggerMapRedraw()
{
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
    if (activeTile != MAP || lv_subject_get_int(&subject_map_state) != MAP_MODE_FOLLOW)
        return;

    lv_async_call(async_map_update_cb, NULL);
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
    
    if (abs(newHeading - global_last_heading) > MAP_HEADING_THRESHOLD)
    {
        global_last_heading = newHeading;
        lv_async_call(async_map_update_cb, NULL);
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
 * @param startPt  First trackData index of the visible window.
 * @param endPt    Last  trackData index of the visible window.
 */
static void buildClimbProfile(int startPt, int endPt)
{
    if (climbCanvas == NULL || trackData.size() < 2)
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

    float distStart = trackData[startPt].accumDist;
    float distEnd   = trackData[endPt].accumDist;
    float distRange = distEnd - distStart;
    if (distRange < 1.0f)
        return;

    float minEle = trackData[startPt].ele;
    float maxEle = trackData[startPt].ele;
    for (int i = startPt + 1; i <= endPt; ++i)
    {
        if (trackData[i].ele < minEle)
            minEle = trackData[i].ele;
        if (trackData[i].ele > maxEle)
            maxEle = trackData[i].ele;
    }
    float eleRange = maxEle - minEle;
    if (eleRange < 1.0f)
        eleRange = 1.0f;

    climbState.minEle   = minEle;
    climbState.maxEle   = maxEle;
    climbState.eleRange = eleRange;

    const auto &segs = climbAnalyzer.segments();
    auto toCol = [](uint32_t rgb) { return lgfx::rgb888_t((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF); };

    uint8_t *alphaBuf = climbState.buf + W * 2 * H;
    memset(alphaBuf, 0x00, W * H);

    // Linear cursor through trackData — O(n) total across all W columns
    int trkCursor = startPt;

    for (int x = 0; x < W; ++x)
    {
        float colDist = distStart + (float)x / (float)(W - 1) * distRange;

        while (trkCursor < endPt - 1 && trackData[trkCursor + 1].accumDist < colDist)
            ++trkCursor;

        float d0 = trackData[trkCursor].accumDist;
        float d1 = trackData[trkCursor + 1].accumDist;
        float e0 = trackData[trkCursor].ele;
        float e1 = trackData[trkCursor + 1].ele;
        float t  = (d1 > d0) ? (colDist - d0) / (d1 - d0) : 0.0f;
        float ele = e0 + t * (e1 - e0);

        int yTop = calcYTop(ele, minEle, eleRange, H);

        lgfx::rgb888_t col = toCol(0x00C800u);
        for (const auto &seg : segs)
        {
            if (colDist >= trackData[seg.startIdx].accumDist &&
                colDist <= trackData[seg.endIdx].accumDist)
            {
                // Local grade over a 50m window centred on colDist
                float wHalf = 25.0f;
                float dA = colDist - wHalf;
                float dB = colDist + wHalf;
                if (dA < trackData[seg.startIdx].accumDist)
                    dA = trackData[seg.startIdx].accumDist;
                if (dB > trackData[seg.endIdx].accumDist)
                    dB = trackData[seg.endIdx].accumDist;
                int maxIdx = (int)trackData.size() - 1;
                int ia = trkCursor;
                while (ia > 0 && trackData[ia].accumDist > dA) --ia;
                while (ia < maxIdx && trackData[ia + 1].accumDist < dA) ++ia;
                float ta = (ia < maxIdx && trackData[ia + 1].accumDist > trackData[ia].accumDist)
                           ? (dA - trackData[ia].accumDist) / (trackData[ia + 1].accumDist - trackData[ia].accumDist)
                           : 0.0f;
                float eleA = trackData[ia].ele + ta * (ia < maxIdx ? (trackData[ia + 1].ele - trackData[ia].ele) : 0.0f);
                int ib = ia;
                while (ib < maxIdx && trackData[ib + 1].accumDist < dB) ++ib;
                float tb = (ib < maxIdx && trackData[ib + 1].accumDist > trackData[ib].accumDist)
                           ? (dB - trackData[ib].accumDist) / (trackData[ib + 1].accumDist - trackData[ib].accumDist)
                           : 0.0f;
                float eleB = trackData[ib].ele + tb * (ib < maxIdx ? (trackData[ib + 1].ele - trackData[ib].ele) : 0.0f);
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
    {
        lv_obj_clear_flag(climbOverlay, LV_OBJ_FLAG_HIDDEN);
    }
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
    const std::vector<ClimbSegment>& segs = climbAnalyzer.segments();
    float curDistObs = trackData[(int)activeIdx].accumDist;
    const ClimbSegment* seg = nullptr;
    for (const ClimbSegment& s : segs)
    {
        float segStartDist = trackData[s.startIdx].accumDist;
        float segEndDist   = trackData[s.endIdx].accumDist;
        if (curDistObs <= segEndDist && segStartDist - curDistObs <= CLIMB_ANTICIPATION_M)
        {
            seg = &s;
            break;
        }
    }
    if (seg == nullptr)
        return;

    float preStartDist = trackData[seg->startIdx].accumDist - CLIMB_ANTICIPATION_M;
    int startPt = seg->startIdx;
    while (startPt > 0 && trackData[startPt - 1].accumDist >= preStartDist)
        --startPt;

    int endPt  = seg->endIdx;
    float dRange = trackData[endPt].accumDist - trackData[startPt].accumDist;
    float dPos   = curDistObs - trackData[startPt].accumDist;
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
    float curEleObs = trackData[(int)activeIdx].ele;
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
    lv_async_call(async_map_update_cb, NULL);
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
        mapView.centerOnGps(gps.gpsData.latitude, gps.gpsData.longitude);
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

        float currentFriction = mapView.isRendering() ? MAP_INERTIA_FRICTION : mapView.friction;
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
    if (canScrollMap)
    {
        lv_event_code_t code = lv_event_get_code(event);
        lv_indev_t * indev = lv_event_get_indev(event);
        lv_point_t p;

        switch (code)
        {
            case LV_EVENT_PRESSED:
                lv_indev_get_point(indev, &p);
                scrollState.last_x      = p.x;
                scrollState.last_y      = p.y;
                scrollState.last_time   = (uint32_t)(esp_timer_get_time() / 1000);
                scrollState.dragStarted = false;
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
    int wptDistance = (int)calcDist(gps.gpsData.latitude, gps.gpsData.longitude, loadWpt.lat, loadWpt.lon);
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
        float wptCourse = calcCourse(gps.gpsData.latitude, gps.gpsData.longitude, loadWpt.lat, loadWpt.lon) - navHeading;
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
    lv_obj_set_size(btnToggle3D, 60, 60);
    lv_obj_clear_flag(btnToggle3D, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(btnToggle3D, &styleMapWidget, 0);
    lv_obj_add_flag(btnToggle3D, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_FLOATING));
    lv_obj_align(btnToggle3D, LV_ALIGN_TOP_RIGHT, 0, 170);
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

    #ifdef BOARD_HAS_PSRAM
        #ifndef TDECK_ESP32S3
            createSatRadar(satTrackTile);
            lv_obj_set_pos(satRadar, (TFT_WIDTH / 2) - canvasCenter_X, 240);
        #endif
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
