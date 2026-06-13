/**
 * @file optionsPanel.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Options Panel
 * @version 0.2.9
 * @date 2026-06
 */

#include "optionsPanel.hpp"
#include "menu.h"

extern Maps mapView;
bool isWaypointOpt  = false;
bool isTrackOpt     = false;
bool isBarOpen      = false;
bool isSubPanelOpen = false;
lv_obj_t *settingsScreen;
lv_obj_t *optionsPanel;
lv_obj_t *menuBtn;
lv_obj_t *optionsScrim;

static lv_obj_t *subPanel;

/**
 * @brief Handles main panel events, triggering actions based on the selected option.
 *
 * @param event Pointer to the LVGL event structure.
 */
void optionsPanelEvent(lv_event_t *event)
{
    char *action = (char *)lv_event_get_user_data(event);
    if (strcmp(action, "addwpt") == 0)
    {
        closeOptionsPanel();
        gpxAction = WPT_ADD;
        isMainScreen = false;
        mapView.redrawMap = false;
        lv_textarea_set_text(gpxTagValue, "");
        isScreenRotated = false;
        lv_obj_set_width(gpxTagValue, tft.width() - 10);
        updateWaypoint(gpxAction);
        lv_screen_load(gpxDetailScreen);
    }
    else if (strcmp(action, "waypoint") == 0)
    {
        isWaypointOpt = true;
        isTrackOpt    = false;
        closeOptionsPanel();
    }
    else if (strcmp(action, "track") == 0)
    {
        isTrackOpt    = true;
        isWaypointOpt = false;
        closeOptionsPanel();
    }
    else if (strcmp(action, "settings") == 0)
    {
        closeOptionsPanel();
        isMainScreen = false;
        lv_screen_load(settingsScreen);
    }
}

/**
 * @brief Handles option events, triggering actions such as load, edit, or delete based on the selected option.
 *
 * @param event Pointer to the LVGL event structure.
 */
void optionEvent(lv_event_t *event)
{
    char *action = (char *)lv_event_get_user_data(event);

    if (strcmp(action, "load") == 0)
    {
        gpxAction    = GPX_LOAD;
        isMainScreen = false;
    }
    else if (strcmp(action, "edit") == 0)
    {
        gpxAction    = GPX_EDIT;
        isMainScreen = false;
    }
    else if (strcmp(action, "delete") == 0)
    {
        gpxAction    = GPX_DEL;
        isMainScreen = false;
    }

    closeSubPanel();

    if (gpxAction == GPX_LOAD || gpxAction == GPX_EDIT || gpxAction == GPX_DEL)
    {
        updateGpxListScreen();
        if (isTrackOpt)
            lv_table_set_cell_value(listGPXScreen, 0, 0, LV_SYMBOL_LEFT " Tracks");
        if (isWaypointOpt)
            lv_table_set_cell_value(listGPXScreen, 0, 0, LV_SYMBOL_LEFT " Waypoints");
        lv_screen_load(listGPXScreen);
    }

    isWaypointOpt  = false;
    isTrackOpt     = false;
    mapView.redrawMap = true;
}

/**
 * @brief Vertical slide animation for the bottom sheet panel.
 *
 * @param var Pointer to the bottom sheet object.
 * @param v Current y position in pixels.
 */
void slideAnim(void *var, int32_t v)
{
    lv_obj_set_y((lv_obj_t *)var, v);
}

/**
 * @brief Resets isScrollingMap once an open animation has finished.
 *
 * @param a Pointer to the finished animation.
 */
static void openAnimDone(lv_anim_t *a)
{
    isScrollingMap = false;
}

/**
 * @brief Hides the scrim once the close animation has finished.
 *
 * @param a Pointer to the finished animation.
 */
static void closeAnimDone(lv_anim_t *a)
{
    isScrollingMap = false;
    lv_obj_add_flag(optionsScrim, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief Opens the sub-panel after the main panel close animation completes.
 *
 * @param a Pointer to the finished animation.
 */
static void mainCloseThenSubOpen(lv_anim_t *a)
{
    isScrollingMap = false;
    lv_obj_add_flag(optionsScrim, LV_OBJ_FLAG_HIDDEN);
    if (isWaypointOpt || isTrackOpt)
        openSubPanel();
}

/**
 * @brief Reopens the main options panel after the sub-panel close animation completes.
 *
 * @param a Pointer to the finished animation.
 */
static void subCloseThenMainOpen(lv_anim_t *a)
{
    isScrollingMap = false;
    lv_obj_add_flag(subPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(optionsScrim, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(optionsScrim, LV_OBJ_FLAG_CLICKABLE);
    isWaypointOpt = false;
    isTrackOpt    = false;
    openOptionsPanel();
}

/**
 * @brief Hides the sub-panel once its close animation has finished.
 *
 * @param a Pointer to the finished animation.
 */
static void subCloseDone(lv_anim_t *a)
{
    isScrollingMap = false;
    lv_obj_add_flag(subPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(optionsScrim, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(optionsScrim, LV_OBJ_FLAG_CLICKABLE);
}

/**
 * @brief Opens the options panel, sliding it up and showing the scrim.
 */
void openOptionsPanel()
{
    if (isBarOpen)
        return;
    isBarOpen      = true;
    isScrollingMap = true;
    lv_obj_clear_flag(optionsScrim, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_y(optionsPanel, TFT_HEIGHT);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, optionsPanel);
    lv_anim_set_exec_cb(&a, slideAnim);
    int32_t margin = (int)(16 * scaleBut);
    lv_anim_set_values(&a, TFT_HEIGHT, margin);
    lv_anim_set_duration(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_completed_cb(&a, openAnimDone);
    lv_anim_start(&a);
}

/**
 * @brief Closes the options panel, sliding it down.
 *        If waypoint/track was selected, opens the sub-panel after close.
 */
void closeOptionsPanel()
{
    if (!isBarOpen)
        return;
    isBarOpen      = false;
    isScrollingMap = true;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, optionsPanel);
    lv_anim_set_exec_cb(&a, slideAnim);
    lv_anim_set_values(&a, lv_obj_get_y(optionsPanel), TFT_HEIGHT);
    lv_anim_set_duration(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    if (isWaypointOpt || isTrackOpt)
        lv_anim_set_completed_cb(&a, mainCloseThenSubOpen);
    else
        lv_anim_set_completed_cb(&a, closeAnimDone);
    lv_anim_start(&a);
}

/**
 * @brief Opens the sub-panel (waypoint/track actions), sliding it up from the bottom.
 */
void openSubPanel()
{
    if (isSubPanelOpen)
        return;
    isSubPanelOpen = true;
    isScrollingMap = true;
    lv_obj_clear_flag(optionsScrim, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(subPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(optionsScrim, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_y(subPanel, TFT_HEIGHT);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, subPanel);
    lv_anim_set_exec_cb(&a, slideAnim);
    int32_t margin = (int)(16 * scaleBut);
    lv_anim_set_values(&a, TFT_HEIGHT, TFT_HEIGHT - lv_obj_get_height(subPanel) - margin);
    lv_anim_set_duration(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_completed_cb(&a, openAnimDone);
    lv_anim_start(&a);
}

/**
 * @brief Closes the sub-panel, sliding it down.
 */
void closeSubPanel()
{
    if (!isSubPanelOpen)
        return;
    isSubPanelOpen = false;
    isScrollingMap = true;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, subPanel);
    lv_anim_set_exec_cb(&a, slideAnim);
    lv_anim_set_values(&a, lv_obj_get_y(subPanel), TFT_HEIGHT);
    lv_anim_set_duration(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_completed_cb(&a, subCloseDone);
    lv_anim_start(&a);
}

/**
 * @brief Toggles the options panel when the menu button is clicked.
 *
 * @param e Pointer to the LVGL event structure.
 */
void hideShowEvent(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        if (isSubPanelOpen)
        {
            isSubPanelOpen = false;
            isScrollingMap = true;
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, subPanel);
            lv_anim_set_exec_cb(&a, slideAnim);
            lv_anim_set_values(&a, lv_obj_get_y(subPanel), TFT_HEIGHT);
            lv_anim_set_duration(&a, 200);
            lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
            lv_anim_set_completed_cb(&a, subCloseThenMainOpen);
            lv_anim_start(&a);
        }
        else if (isBarOpen)
            closeOptionsPanel();
        else
            openOptionsPanel();
    }
}

/**
 * @brief Closes the options panel or sub-panel when the scrim is clicked.
 *
 * @param e Pointer to the LVGL event structure.
 */
static void scrimEvent(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        if (isSubPanelOpen)
        {
            isWaypointOpt = false;
            isTrackOpt    = false;
            closeSubPanel();
        }
        else
            closeOptionsPanel();
    }
}

/**
 * @brief Creates an icon+label cell inside a panel.
 *
 * @param parent   Parent container.
 * @param iconSrc  Path to the .bin icon file.
 * @param label    Text label shown below the icon.
 * @param action   User data string passed to the event callback.
 * @param cb       Event callback (optionsPanelEvent or optionEvent).
 * @param cellW    Cell width in pixels.
 * @param cellH    Cell height in pixels.
 */
static void createOptionCell(lv_obj_t *parent, const char *iconSrc, const char *label, char *action,
                             lv_event_cb_t cb, int32_t cellW, int32_t cellH)
{
    int32_t iconSize = (int)(cellW * 0.55f);

    lv_obj_t *cell = lv_obj_create(parent);
    lv_obj_remove_style_all(cell);
    lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, 0);
    lv_obj_set_size(cell, cellW, cellH);
    lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(cell, (int)(4 * scaleBut), 0);
    lv_obj_set_style_pad_gap(cell, (int)(4 * scaleBut), 0);
    lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cell, cb, LV_EVENT_PRESSED, action);

    lv_obj_t *img = lv_img_create(cell);
    lv_img_set_src(img, iconSrc);
    lv_img_set_zoom(img, buttonScale);
    lv_obj_update_layout(img);
    lv_obj_set_style_size(img, iconSize, iconSize, 0);

    lv_obj_t *lbl = lv_label_create(cell);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_font(lbl, fontSmall, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_width(lbl, cellW - (int)(8 * scaleBut));
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
}

/**
 * @brief Creates and configures the floating options panel with menu FAB and action cells.
 */
void createOptionsPanel()
{
    // Full screen dim scrim, shared by both panels
    optionsScrim = lv_obj_create(mainScreen);
    lv_obj_remove_style_all(optionsScrim);
    lv_obj_add_style(optionsScrim, &styleScrim, 0);
    lv_obj_set_size(optionsScrim, TFT_WIDTH, TFT_HEIGHT);
    lv_obj_set_pos(optionsScrim, 0, 0);
    lv_obj_clear_flag(optionsScrim, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(optionsScrim, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_add_flag(optionsScrim, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(optionsScrim, scrimEvent, LV_EVENT_CLICKED, NULL);

    // Layout constants
    int32_t margin = (int)(16 * scaleBut);
    int32_t pad    = (int)(12 * scaleBut);
    int32_t panelW = TFT_WIDTH - margin * 2;
    int32_t panelH = TFT_HEIGHT - margin * 2;

    // Main panel cell sizing: 3 cols, fall back to 2 if too narrow
    int32_t cols  = 3;
    int32_t cellW = (panelW - pad * 2 - pad * (cols - 1)) / cols;
    if (cellW < 72)
    {
        cols  = 2;
        cellW = (panelW - pad * 2 - pad * (cols - 1)) / cols;
    }
    int32_t cellH = cellW;

    // Main options panel
    optionsPanel = lv_obj_create(mainScreen);
    lv_obj_remove_style_all(optionsPanel);
    lv_obj_add_style(optionsPanel, &styleBottomSheet, 0);
    lv_obj_set_flex_flow(optionsPanel, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(optionsPanel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(optionsPanel, pad, 0);
    lv_obj_set_style_pad_gap(optionsPanel, pad, 0);
    lv_obj_set_size(optionsPanel, panelW, panelH);
    lv_obj_set_pos(optionsPanel, margin, TFT_HEIGHT);
    lv_obj_clear_flag(optionsPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(optionsPanel, LV_OBJ_FLAG_FLOATING);

    createOptionCell(optionsPanel, addWptIconFile,   "Add Waypoint", (char*)"addwpt",   optionsPanelEvent, cellW, cellH);
    createOptionCell(optionsPanel, waypointIconFile, "Waypoints",    (char*)"waypoint", optionsPanelEvent, cellW, cellH);
    createOptionCell(optionsPanel, trackIconFile,    "Tracks",       (char*)"track",    optionsPanelEvent, cellW, cellH);
    createOptionCell(optionsPanel, settingsIconFile, "Settings",     (char*)"settings", optionsPanelEvent, cellW, cellH);

    // Sub-panel cell sizing: same criteria as main panel
    int32_t subCols  = 3;
    int32_t subCellW = (panelW - pad * 2 - pad * (subCols - 1)) / subCols;
    if (subCellW < 72)
    {
        subCols  = 2;
        subCellW = (panelW - pad * 2 - pad * (subCols - 1)) / subCols;
    }
    int32_t subCellH = subCellW;
    int32_t subRows  = (3 + subCols - 1) / subCols;
    int32_t subH     = subCellH * subRows + pad * (subRows - 1) + pad * 2;

    subPanel = lv_obj_create(mainScreen);
    lv_obj_remove_style_all(subPanel);
    lv_obj_add_style(subPanel, &styleBottomSheet, 0);
    lv_obj_set_flex_flow(subPanel, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(subPanel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(subPanel, pad, 0);
    lv_obj_set_style_pad_gap(subPanel, pad, 0);
    lv_obj_set_size(subPanel, panelW, subH);
    lv_obj_set_pos(subPanel, margin, TFT_HEIGHT);
    lv_obj_clear_flag(subPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(subPanel, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_HIDDEN));

    createOptionCell(subPanel, loadIconFile,   "Load",   (char*)"load",   optionEvent, subCellW, subCellH);
    createOptionCell(subPanel, editIconFile,   "Edit",   (char*)"edit",   optionEvent, subCellW, subCellH);
    createOptionCell(subPanel, deleteIconFile, "Delete", (char*)"delete", optionEvent, subCellW, subCellH);

    // Floating menu button (FAB)
    int32_t fabSize  = (int)(44 * scaleBut);
    int32_t iconSize = (int)(24 * scaleBut);
    lv_obj_t *menuBtnWrap = lv_obj_create(mainScreen);
    lv_obj_remove_style_all(menuBtnWrap);
    lv_obj_set_style_bg_color(menuBtnWrap, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(menuBtnWrap, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(menuBtnWrap, 0, 0);
    lv_obj_set_style_radius(menuBtnWrap, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(menuBtnWrap, 0, 0);
    lv_obj_set_size(menuBtnWrap, fabSize, fabSize);
    lv_obj_add_flag(menuBtnWrap, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_clear_flag(menuBtnWrap, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(menuBtnWrap, LV_ALIGN_BOTTOM_RIGHT, -4, -4);
    lv_obj_add_event_cb(menuBtnWrap, hideShowEvent, LV_EVENT_ALL, NULL);
    menuBtn = menuBtnWrap;
    LV_IMG_DECLARE(menu);
    lv_obj_t *menuIcon = lv_img_create(menuBtnWrap);
    lv_img_set_src(menuIcon, &menu);
    lv_img_set_zoom(menuIcon, buttonScale);
    lv_obj_update_layout(menuIcon);
    lv_obj_set_size(menuIcon, iconSize, iconSize);
    lv_obj_center(menuIcon);
}
