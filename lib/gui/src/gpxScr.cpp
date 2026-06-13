/**
 * @file gpxScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - GPX list screen
 * @version 0.2.9
 * @date 2026-06
 */

#include "gpxScr.hpp"
#include "lv_subjects.hpp"
#include "router.hpp"
#include "climbAnalyzer.hpp"
#include "gps.hpp"
#include "gpsMath.hpp"

extern Maps mapView;
extern Storage storage;
extern Gps gps;
extern wayPoint loadWpt;
extern bool isWaypointOpt;
extern bool isTrackOpt;
bool gpxWaypoint = false;
bool gpxTrack = false;
bool isTrackLoaded = false;

extern TrackVector trackData;             /**< Vector containing track waypoints */
extern std::vector<TurnPoint> turnPoints; /**< Vector containing turn points */
extern ClimbAnalyzer climbAnalyzer;       /**< Climb profile analyzer */
extern NavState navState;

lv_obj_t *listGPXScreen;                /**< Add Waypoint screen */
static bool longPressHandled = false;   /**< Guard to prevent repeated long-press action per gesture */

/**
 * @brief Loads a GPX waypoint or track from the selected file.
 *
 * @param gpx    Initialised GPXParser for the selected file.
 * @param gpxName Name of the selected entry (without icon prefix).
 */
static void handleGpxLoad(GPXParser &gpx, const char *gpxName)
{
    showMsg(LV_SYMBOL_DOWNLOAD, " Loading data...");
    if (gpxWaypoint)
    {
        loadWpt = gpx.getWaypointInfo(gpxName);
        LV_IMG_DECLARE(navup);
        lv_img_set_src(arrowNav, &navup);

        if (loadWpt.lat != 0 && loadWpt.lon != 0)
        {
            isTrackLoaded = false;
            trackData.clear();
            trackData.shrink_to_fit();
            navState = NavState{};
            resetNavigationUI();
            mapView.redrawTrack();

            lv_obj_clear_flag(navTile, LV_OBJ_FLAG_HIDDEN);
            if (mapSet.vectorMap)
                lv_obj_clear_flag(btnToggle3D, LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(btnToggle3D, LV_OBJ_FLAG_HIDDEN);

            lv_label_set_text_fmt(latNav, "%s", latFormatString(loadWpt.lat));
            lv_label_set_text_fmt(lonNav, "%s", lonFormatString(loadWpt.lon));
            lv_label_set_text_fmt(nameNav, "%s", loadWpt.name);

            mapView.setWaypoint(loadWpt.lat, loadWpt.lon);
            mapView.updateMap();

            routeDstLat = loadWpt.lat;
            routeDstLon = loadWpt.lon;
            lv_subject_set_int(&subject_rerouting, 1);
            rerouteRequested.store(true);

            lv_obj_send_event(mapTile, LV_EVENT_REFRESH, NULL);
        }
        else
        {
            lv_obj_add_flag(navTile, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(btnToggle3D, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (gpxTrack)
    {
        isTrackLoaded = false;
        trackData.clear();
        trackData.shrink_to_fit();
        navState = NavState{};
        gps.resetSimulation();
        resetNavigationUI();
        climbAnalyzer.clear();
        gpx.loadTrack(trackData);

        size_t gpxStartIdx = 0;
        if (!trackData.empty())
        {
            float distToStart = calcDist(gps.gpsData.latitude, gps.gpsData.longitude,
                                         trackData[0].lat, trackData[0].lon);
            if (distToStart > 50.0f)
            {
                TrackVector approachRoute;
                RouterResult res = router.route(gps.gpsData.latitude, gps.gpsData.longitude,
                                                trackData[0].lat, trackData[0].lon, approachRoute);
                if (res == RouterResult::OK && !approachRoute.empty())
                {
                    gpxStartIdx = approachRoute.size() - 1;
                    approachRoute.pop_back();
                    approachRoute.insert(approachRoute.end(), trackData.begin(), trackData.end());
                    trackData = std::move(approachRoute);
                }
            }

            if (gpxStartIdx > 0)
            {
                // Recalculate accumDist only for the approach segment, then offset
                // the GPX points so the full track has a continuous distance axis.
                trackData[0].accumDist = 0.0f;
                for (size_t i = 1; i <= gpxStartIdx; ++i)
                {
                    float d = calcDist(trackData[i-1].lat, trackData[i-1].lon,
                                       trackData[i].lat,   trackData[i].lon);
                    trackData[i].accumDist = trackData[i-1].accumDist + d;
                }
                float approachDist = trackData[gpxStartIdx].accumDist;
                for (size_t i = gpxStartIdx + 1; i < trackData.size(); ++i)
                    trackData[i].accumDist += approachDist;
            }
        }

        if (mapSet.showClimb)
        {
            TrackVector gpxOnly(trackData.begin() + gpxStartIdx, trackData.end());
            climbAnalyzer.analyze(gpxOnly, (int)gpxStartIdx);
        }
        turnPoints = gpx.getTurnPointsSlidingWindow(18.0f, 10, 70.0f, 5, trackData);
        isTrackLoaded = !trackData.empty();
        if (isTrackLoaded && mapSet.vectorMap)
            lv_obj_clear_flag(btnToggle3D, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(turnByTurn, LV_OBJ_FLAG_HIDDEN);
        mapView.updateMap();
        mapView.redrawTrack();
        lv_obj_send_event(mapTile, LV_EVENT_REFRESH, NULL);
    }
    closeMsg();
    loadMainScreen();
}

/**
 * @brief Opens the GPX detail screen to edit the name of the selected waypoint or track.
 *
 * @param gpx    Initialised GPXParser for the selected file.
 * @param gpxName Name of the selected entry (without icon prefix).
 */
static void handleGpxEdit(GPXParser &gpx, const char *gpxName)
{
    isMainScreen = false;
    mapView.redrawMap = false;

    if (gpxWaypoint)
    {
        loadWpt = gpx.getWaypointInfo(gpxName);
        lv_textarea_set_text(gpxTagValue, loadWpt.name);
        lv_label_set_text_static(gpxTag, LV_SYMBOL_LEFT " Waypoint Name:");
        lv_obj_clear_flag(labelLat, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(labelLatValue, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(labelLon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(labelLonValue, LV_OBJ_FLAG_HIDDEN);
    }

    if (gpxTrack)
    {
        loadWpt.name = strdup(gpxName);
        lv_textarea_set_text(gpxTagValue, loadWpt.name);
        lv_label_set_text_static(gpxTag, LV_SYMBOL_LEFT " Track Name:");
        lv_obj_add_flag(labelLat, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(labelLatValue, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(labelLon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(labelLonValue, LV_OBJ_FLAG_HIDDEN);
    }

    isScreenRotated = false;
    lv_obj_set_width(gpxTagValue, tft.width() - 10);
    updateWaypoint(gpxAction);
    lv_screen_load(gpxDetailScreen);
}

/**
 * @brief Deletes the selected GPX waypoint or track from its file.
 *
 * @param gpx    Initialised GPXParser for the selected file.
 * @param gpxName Name of the selected entry (without icon prefix).
 */
static void handleGpxDelete(GPXParser &gpx, const char *gpxName)
{
    if (gpxWaypoint)
        gpx.deleteTagByName(gpxWaypointTag, gpxName);
    if (gpxTrack)
        gpx.deleteTagByName(gpxTrackTag, gpxName);
    loadMainScreen();
}

/**
 * @brief Waypoint list event handler. Handles long-press events on the GPX waypoint list for load, edit, or delete actions.
 *
 * @param event LVGL event pointer.
 */
void gpxListEvent(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(event);
    uint32_t row;
    uint32_t col;

    if (code == LV_EVENT_RELEASED)
        longPressHandled = false;

    if (code == LV_EVENT_LONG_PRESSED && !longPressHandled)
    {
        longPressHandled = true;
        lv_table_get_selected_cell(obj, &row, &col);

        if (row != 0)
        {
            const char *sel = lv_table_get_cell_value(obj, row, col);
            const char *gpxName = (strlen(sel) > 6) ? sel + 6 : sel;
            const char *gpxFile = lv_table_get_cell_value(obj, row, col + 1);

            if (gpxWaypoint)
                snprintf(gpxFileFolder, sizeof(gpxFileFolder), "%s/%s", wptFolder, gpxFile);
            else if (gpxTrack)
                snprintf(gpxFileFolder, sizeof(gpxFileFolder), "%s/%s", trkFolder, gpxFile);

            GPXParser gpx(gpxFileFolder);

            if (sel != nullptr && sel[0] != '\0')
            {
                switch (gpxAction)
                {
                    case GPX_LOAD:   handleGpxLoad(gpx, gpxName);   break;
                    case GPX_EDIT:   handleGpxEdit(gpx, gpxName);   break;
                    case GPX_DEL:    handleGpxDelete(gpx, gpxName); break;
                    default:         break;
                }
            }
        }
        else if (row == 0)
        {
            lv_obj_add_flag(navTile, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(btnToggle3D, LV_OBJ_FLAG_HIDDEN);
            loadMainScreen();
        }
    }
}

/**
 * @brief Create List Waypoint Screen
 *
 * @details Initializes the GPX Waypoint List screen with a two-column table,
 * 			sets up column widths, headers, styles, and attaches the event callback for handling user actions.
 */
void createGpxListScreen()
{
    listGPXScreen = lv_table_create(NULL);
    lv_table_set_col_cnt(listGPXScreen, 2);
    lv_table_set_column_width(listGPXScreen,1,400);
    lv_obj_set_size(listGPXScreen, TFT_WIDTH, TFT_HEIGHT);
    lv_obj_set_style_text_font(listGPXScreen, fontMedium, 0);
    lv_table_set_cell_value(listGPXScreen, 0, 0, LV_SYMBOL_LEFT " Waypoints");
    lv_table_set_cell_value(listGPXScreen, 0, 1, LV_SYMBOL_FILE " File");
    lv_table_set_column_width(listGPXScreen, 0, TFT_WIDTH);
    lv_obj_add_event_cb(listGPXScreen, gpxListEvent, LV_EVENT_ALL, NULL);
    lv_obj_set_style_pad_ver(listGPXScreen, 15, LV_PART_ITEMS);
    lv_obj_set_style_border_width(listGPXScreen, 1, LV_PART_ITEMS);
    lv_obj_set_style_border_color(listGPXScreen, lv_color_hex(0x303030), LV_PART_ITEMS);
    lv_obj_set_style_border_side(listGPXScreen, LV_BORDER_SIDE_BOTTOM, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(listGPXScreen, lv_color_hex(0x303030), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(listGPXScreen, LV_OPA_100, LV_PART_ITEMS | LV_STATE_PRESSED);
}

/**
 * @brief Update List Waypoint Screen from file.
 *
 * @details Refreshes the GPX list screen by loading and displaying waypoints or tracks from the corresponding files and folders.
 */
void updateGpxListScreen()
{
    longPressHandled = false;
    lv_obj_clean(listGPXScreen);
    lv_table_set_row_count(listGPXScreen, 1);
    isMainScreen = false;
    showMsg(LV_SYMBOL_DOWNLOAD," Getting files...");
    if (isWaypointOpt)
    {
        gpxWaypoint = true;
        gpxTrack = false;
        uint16_t totalGpx = 1;
        std::map<std::string, std::vector<std::string>> waypointByFile = GPXParser::getTagElementList(gpxWaypointTag, gpxNameElem, wptFolder);
        for (std::map<std::string, std::vector<std::string>>::const_iterator it = waypointByFile.begin(); it != waypointByFile.end(); ++it)
        {
            const std::string& fileName = it->first;
            const std::vector<std::string>& waypointNames = it->second;
            for (const std::string& gpxTagValue : waypointNames)
            {
                lv_table_set_cell_value_fmt(listGPXScreen, totalGpx, 0, LV_SYMBOL_GPS " - %s", gpxTagValue.c_str());
                lv_table_set_cell_value_fmt(listGPXScreen, totalGpx, 1, "%s", fileName.c_str());
                totalGpx++;
            }
        }
    }
    if (isTrackOpt)
    {
        gpxWaypoint = false;
        gpxTrack = true;
        uint16_t totalGpx = 1;
        std::map<std::string, std::vector<std::string>> tracksByFile = GPXParser::getTagElementList(gpxTrackTag, gpxNameElem, trkFolder);
        for (std::map<std::string, std::vector<std::string>>::const_iterator it = tracksByFile.begin(); it != tracksByFile.end(); ++it)
        {
            const std::string& fileName = it->first;
            const std::vector<std::string>& trackNames = it->second;
            for (const std::string& trackName : trackNames)
            {
                lv_table_set_cell_value_fmt(listGPXScreen, totalGpx, 0, LV_SYMBOL_SHUFFLE " - %s", trackName.c_str());
                lv_table_set_cell_value_fmt(listGPXScreen, totalGpx, 1, "%s", fileName.c_str());
                totalGpx++;
            }
        }
    }
    closeMsg();
}
