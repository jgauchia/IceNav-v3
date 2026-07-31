/**
 * @file satInfoScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Satellite info screen 
 * @version 0.3.0
 * @date 2026-06
 */
#include "satInfoScr.hpp"
#include "lv_subjects.hpp"
#include "mainScr.hpp"

lv_obj_t *infoGrid;
lv_obj_t *pdopLabel;
lv_obj_t *hdopLabel;
lv_obj_t *vdopLabel;
lv_obj_t *altLabel;
lv_obj_t *satRadar;
lv_obj_t *satelliteBar;               
lv_chart_series_t *satelliteBarSerie; 
lv_obj_t *constMsg;
extern Gps gps;

/**
 * @brief Observer callback for DOP labels (PDOP, HDOP, VDOP).
 *
 * @param observer LVGL observer pointer; target is the label object.
 * @param subject  Subject carrying the DOP value scaled by 10.
 *
 * @details user_data must be a string literal with the prefix ("PDOP", "HDOP" or "VDOP").
 */
static void dop_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t *label = (lv_obj_t *)lv_observer_get_target(observer);
    const char *prefix = (const char *)lv_observer_get_user_data(observer);
    lv_label_set_text_fmt(label, "%s: %.1f", prefix, lv_subject_get_int(subject) / 10.0f);
}

/**
 * @brief Observer callback for Altitude label
 */
static void alt_sat_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    lv_obj_t * label = (lv_obj_t *)lv_observer_get_target(observer);
    lv_label_set_text_fmt(label, "ALT: %4dm.", lv_subject_get_int(subject));
}

/**
 * @brief Async callback to redraw Satellite SNR and Sky charts
 */
static void async_sats_update_cb(void * user_data)
{
    if (activeTile != SATTRACK)
        return;
    drawSatSNR();
    if (satRadar)
        lv_obj_invalidate(satRadar);
}

/**
 * @brief Observer callback for complex satellite data changes
 */
static void sats_data_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    if (activeTile != SATTRACK)
        return;
    lv_async_call(async_sats_update_cb, NULL);
}

/**
 * @brief SNR Bar draw event.
 *
 * @details Handles the drawing of the Signal-to-Noise Ratio (SNR) bar chart for satellites. Colors each bar depending on the GNSS constellation
 * 			and whether the satellite is active. After drawing, overlays signal values and satellite IDs on the chart.
 *
 * @param event LVGL event pointer.
 */
void satelliteBarDrawEvent(lv_event_t * event)
{
    lv_event_code_t e = lv_event_get_code(event);
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(event);
    if (e == LV_EVENT_DRAW_TASK_ADDED) 
    {
        lv_draw_task_t * drawTask = lv_event_get_draw_task(event);
        lv_draw_dsc_base_t * base_dsc = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(drawTask);
        if (base_dsc->part == LV_PART_ITEMS)
        {
            uint16_t dscId = base_dsc->id2;
            if (lv_draw_task_get_type(drawTask) == LV_DRAW_TASK_TYPE_FILL) 
            {
                lv_draw_fill_dsc_t * fill_dsc = lv_draw_task_get_fill_dsc(drawTask);
                if (fill_dsc)
                {
                    if ( strcmp(gps.satTracker[dscId].talker_id,"GP") == 0 )
                        fill_dsc->color = gps.satTracker[dscId].active ? GP_ACTIVE_COLOR : GP_INACTIVE_COLOR;
                    else if ( strcmp(gps.satTracker[dscId].talker_id,"GL") == 0 )
                        fill_dsc->color = gps.satTracker[dscId].active ? GL_ACTIVE_COLOR : GL_INACTIVE_COLOR;
                    else if ( strcmp(gps.satTracker[dscId].talker_id,"BD") == 0 )
                        fill_dsc->color = gps.satTracker[dscId].active ? BD_ACTIVE_COLOR : BD_INACTIVE_COLOR;
                    else
                        // Fallback for GN, GA, etc. using series default (Light Green) or grey if inactive
                        fill_dsc->color = gps.satTracker[dscId].active ? lv_palette_main(LV_PALETTE_LIGHT_GREEN) : lv_palette_main(LV_PALETTE_GREY);
                }
            }
        }
    }
    if (e == LV_EVENT_DRAW_POST_END) 
    {
        lv_layer_t * layer = lv_event_get_layer(event);
        static char label_bufs[MAX_SATELLITES_IN_VIEW][2][8];
        lv_area_t chartObjCoords;
        lv_obj_get_coords(obj, &chartObjCoords);
        for (uint16_t i = 0; i < gps.gpsData.satInView && i < MAX_SATELLITES_IN_VIEW; i++) 
        {
            lv_point_t p;
            lv_chart_get_point_pos_by_id(obj, lv_chart_get_series_next(obj, NULL), i, &p);
            int32_t centerX = chartObjCoords.x1 + p.x;
            if (gps.satTracker[i].snr > 0)
            {
                lv_snprintf(label_bufs[i][0], 8, "%d", gps.satTracker[i].snr);
                lv_draw_label_dsc_t dsc;
                lv_draw_label_dsc_init(&dsc);
                dsc.color = lv_color_white();
                dsc.font = fontSmall;
                dsc.text = label_bufs[i][0];
                dsc.align = LV_TEXT_ALIGN_CENTER;
                lv_area_t a;
                a.x1 = centerX - (int)(20 * scaleSatInfo);
                a.x2 = centerX + (int)(20 * scaleSatInfo);
                a.y1 = chartObjCoords.y1 + p.y - (int)(15 * scaleSatInfo);
                a.y2 = a.y1 + (int)(15 * scaleSatInfo);
                lv_draw_label(layer, &dsc, &a);
            }
            lv_snprintf(label_bufs[i][1], 8, "%d", gps.satTracker[i].satNum);
            lv_draw_label_dsc_t dscId;
            lv_draw_label_dsc_init(&dscId);
            dscId.color = lv_color_white();
            dscId.font = fontSmall;
            dscId.text = label_bufs[i][1];
            dscId.align = LV_TEXT_ALIGN_CENTER;
            lv_area_t aId;
            aId.x1 = centerX - (int)(20 * scaleSatInfo);
            aId.x2 = centerX + (int)(20 * scaleSatInfo);
            aId.y1 = chartObjCoords.y2 - (int)(16 * scaleSatInfo);
            aId.y2 = chartObjCoords.y2;
            lv_draw_label(layer, &dscId, &aId);
        }
    } 
}
/**
 * @brief SNR long press event for showing constellation map (only for T-DECK).
 *
 * @details Handles the long press event on the SNR bar to display the constellation map message, specifically for the T-DECK device.
 *
 * @param event LVGL event pointer.
 */
void constSatEvent(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_LONG_PRESSED)
        lv_obj_clear_flag(constMsg,LV_OBJ_FLAG_HIDDEN); 
}
/**
 * @brief Event for hiding the constellation map (only for T-DECK).
 *
 * @details Handles the long press event to hide the constellation map message on the T-DECK device.
 *
 * @param event LVGL event pointer.
 */
void closeConstSatEvent(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_LONG_PRESSED)
        lv_obj_add_flag(constMsg,LV_OBJ_FLAG_HIDDEN); 
}
/**
 * @brief Satellite radar draw event callback
 *
 * @details Handles direct drawing of the satellite constellation and satellite positions
 *          on the widget layer.
 *
 * @param event LVGL event pointer.
 */
static void sat_radar_draw_cb(lv_event_t * e)
{
    lv_layer_t * layer = lv_event_get_layer(e);
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);
    lv_area_t obj_area;
    lv_obj_get_coords(obj, &obj_area);

    // 1. Draw Constellation (Circles and Lines)
    lv_draw_arc_dsc_t dscArc;
    lv_draw_arc_dsc_init(&dscArc);
    dscArc.color = CONSTEL_COLOR;
    dscArc.width = (int)(2 * scaleSatInfo);
    dscArc.center.x = obj_area.x1 + canvasCenter_X;
    dscArc.center.y = obj_area.y1 + canvasCenter_Y;
    dscArc.start_angle = 0;
    dscArc.end_angle = 360;
    dscArc.radius = canvasRadius;
    lv_draw_arc(layer, &dscArc);
    dscArc.radius = ( canvasRadius * 2 ) / 3;
    lv_draw_arc(layer, &dscArc);
    dscArc.radius = canvasRadius / 3 ;
    lv_draw_arc(layer, &dscArc);

    lv_draw_line_dsc_t dscLine;
    lv_draw_line_dsc_init(&dscLine);
    dscLine.color = CONSTEL_COLOR;
    dscLine.width = (int)(2 * scaleSatInfo);
    dscLine.p1.x = obj_area.x1 + canvasCenter_X;
    dscLine.p1.y = obj_area.y1 + canvasOffset;
    dscLine.p2.x = obj_area.x1 + canvasCenter_X;
    dscLine.p2.y = obj_area.y1 + canvasSize - canvasOffset;
    lv_draw_line(layer, &dscLine);
    dscLine.p1.x = obj_area.x1 + canvasOffset;
    dscLine.p1.y = obj_area.y1 + canvasCenter_Y;
    dscLine.p2.x = obj_area.x1 + canvasSize - canvasOffset;
    dscLine.p2.y = obj_area.y1 + canvasCenter_Y;
    lv_draw_line(layer, &dscLine);

    // 2. Draw Cardinal Directions (outside the outer circle)
    lv_draw_label_dsc_t dscLabel;
    lv_draw_label_dsc_init(&dscLabel);
    dscLabel.color = CONSTEL_COLOR;
    dscLabel.font = fontDefault;
    dscLabel.align = LV_TEXT_ALIGN_CENTER;
    int half_w = (int)(10 * scaleSatInfo);
    int fontAscent = fontDefault->line_height - fontDefault->base_line;
    int fontHalfH = fontDefault->line_height / 2;
    int cardOffset = canvasRadius + 3 + fontAscent - fontHalfH;
    dscLabel.text = "N";
    lv_area_t labelPos = {obj_area.x1 + canvasCenter_X - half_w, obj_area.y1 + canvasCenter_Y - cardOffset - fontHalfH, obj_area.x1 + canvasCenter_X + half_w, obj_area.y1 + canvasCenter_Y - cardOffset + fontHalfH};
    lv_draw_label(layer, &dscLabel, &labelPos);
    dscLabel.text = "S";
    labelPos = {obj_area.x1 + canvasCenter_X - half_w, obj_area.y1 + canvasCenter_Y + cardOffset - fontHalfH, obj_area.x1 + canvasCenter_X + half_w, obj_area.y1 + canvasCenter_Y + cardOffset + fontHalfH};
    lv_draw_label(layer, &dscLabel, &labelPos);
    dscLabel.text = "E";
    labelPos = {obj_area.x1 + canvasCenter_X + cardOffset - half_w, obj_area.y1 + canvasCenter_Y - fontHalfH, obj_area.x1 + canvasCenter_X + cardOffset + half_w, obj_area.y1 + canvasCenter_Y + fontHalfH};
    lv_draw_label(layer, &dscLabel, &labelPos);
    dscLabel.text = "W";
    labelPos = {obj_area.x1 + canvasCenter_X - cardOffset - half_w, obj_area.y1 + canvasCenter_Y - fontHalfH, obj_area.x1 + canvasCenter_X - cardOffset + half_w, obj_area.y1 + canvasCenter_Y + fontHalfH};
    lv_draw_label(layer, &dscLabel, &labelPos);

    // 3. Draw Satellites
    lv_draw_arc_dsc_t dscSat;
    lv_draw_arc_dsc_init(&dscSat);
    dscSat.width = (int)(8 * scaleSatInfo);
    dscSat.start_angle = 0;
    dscSat.end_angle = 360;
    dscSat.radius = (int)(8 * scaleSatInfo);
    dscSat.opa = LV_OPA_70;

    for (int i = 0; i < gps.gpsData.satInView && i < MAX_SATELLITES_IN_VIEW; i++) 
    {
        if ( strcmp(gps.satTracker[i].talker_id,"GP") == 0 )
            dscSat.color = gps.satTracker[i].active ? GP_ACTIVE_COLOR : GP_INACTIVE_COLOR;
        else if ( strcmp(gps.satTracker[i].talker_id,"GL") == 0 )
            dscSat.color = gps.satTracker[i].active ? GL_ACTIVE_COLOR : GL_INACTIVE_COLOR;
        else if ( strcmp(gps.satTracker[i].talker_id,"BD") == 0 )
            dscSat.color = gps.satTracker[i].active ? BD_ACTIVE_COLOR : BD_INACTIVE_COLOR;
        else
            dscSat.color = gps.satTracker[i].active ? lv_palette_main(LV_PALETTE_LIGHT_GREEN) : lv_palette_main(LV_PALETTE_GREY);

        dscSat.center.x = obj_area.x1 + gps.satTracker[i].posX;
        dscSat.center.y = obj_area.y1 + gps.satTracker[i].posY;
        lv_draw_arc(layer, &dscSat);

        static char buf[MAX_SATELLITES_IN_VIEW][8];
        lv_snprintf(buf[i], sizeof(buf[i]), "%d", gps.satTracker[i].satNum);
        lv_draw_label_dsc_t dscSatLabel;
        lv_draw_label_dsc_init(&dscSatLabel);
        dscSatLabel.color = lv_color_white();
        dscSatLabel.font = fontSmall;
        dscSatLabel.text = buf[i];
        dscSatLabel.align = LV_TEXT_ALIGN_CENTER;
        lv_area_t satLabelArea;
        satLabelArea.x1 = dscSat.center.x - (int)(12 * scaleSatInfo);
        satLabelArea.x2 = dscSat.center.x + (int)(12 * scaleSatInfo);
        satLabelArea.y1 = dscSat.center.y - (int)(6 * scaleSatInfo);
        satLabelArea.y2 = dscSat.center.y + (int)(6 * scaleSatInfo);
        lv_draw_label(layer, &dscSatLabel, &satLabelArea);
    }
}

/**
 * @brief Create Satellite Radar Widget
 *
 * @details Initializes and creates the widget for rendering the satellite constellation
 *
 * @param screen Pointer to the LVGL screen object where the radar will be created.
 */
void createSatRadar(_lv_obj_t *screen)
{
    satRadar = lv_obj_create(screen);
    lv_obj_set_size(satRadar, canvasSize, canvasSize);
    lv_obj_set_style_bg_color(satRadar, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(satRadar, LV_OPA_100, 0);
    lv_obj_set_style_border_width(satRadar, 0, 0);
    lv_obj_add_event_cb(satRadar, sat_radar_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
}
/**
 * @brief Satellite info screen
 *
 * @details Creates and lays out the satellite information screen, 
 *
 * @param screen Pointer to the LVGL screen object where the satellite info screen will be created.
 */
void satelliteScr(_lv_obj_t *screen)
{
    lv_obj_t *satContainer = lv_obj_create(screen);
    lv_obj_set_size(satContainer, TFT_WIDTH, TFT_HEIGHT - 25);
    lv_obj_set_flex_flow(satContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(satContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_style(satContainer, &styleTransparent, LV_PART_MAIN);
    lv_obj_set_style_pad_all(satContainer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(satContainer, (int)(5 * scaleSatInfo), LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(satContainer, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t * barCont = lv_obj_create(satContainer);
#ifdef TDECK_ESP32S3
    lv_obj_set_size(barCont, TFT_WIDTH, 145);
#else
    lv_obj_set_size(barCont, TFT_WIDTH, (int)(180 * scaleSatInfo));
#endif
    uint16_t barHeight = 120;
#ifdef TDECK_ESP32S3
    barHeight = 100;
#else
    barHeight = (uint16_t)(120 * scaleSatInfo);
#endif
    lv_obj_t * wrapper = lv_obj_create(barCont);
    lv_obj_remove_style_all(wrapper);
    lv_obj_set_size(wrapper, TFT_WIDTH * 2, barHeight);
    const char* gnssNames[] = {"GPS", "GLONASS", "BEIDOU"};
    lv_color_t activeColors[] = {GP_ACTIVE_COLOR, GL_ACTIVE_COLOR, BD_ACTIVE_COLOR};
    lv_color_t inactiveColors[] = {GP_INACTIVE_COLOR, GL_INACTIVE_COLOR, BD_INACTIVE_COLOR};
    for (int i = 0; i < 3; i++)
    {
        lv_obj_t *gnssLabel = lv_label_create(barCont);
        lv_obj_set_style_text_font(gnssLabel, fontSatInfo, 0);
        lv_obj_set_width(gnssLabel, (int)(90 * scaleSatInfo));
        lv_obj_set_style_bg_color(gnssLabel, activeColors[i], 0);
        lv_obj_set_style_bg_opa(gnssLabel, LV_OPA_100, 0);
        lv_obj_set_style_border_color(gnssLabel, inactiveColors[i], 0);
        lv_obj_set_style_border_width(gnssLabel, 1, 0);
        lv_obj_set_style_border_opa(gnssLabel, LV_OPA_100, 0);
        lv_label_set_text(gnssLabel, gnssNames[i]);
        lv_obj_set_style_text_align(gnssLabel, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_pos(gnssLabel, i * (int)(95 * scaleSatInfo), barHeight + (int)(7 * scaleSatInfo));
    }
    satelliteBar = lv_chart_create(wrapper);
    lv_obj_set_size(satelliteBar, TFT_WIDTH * 2, barHeight);
    lv_chart_set_div_line_count(satelliteBar, 10, 0);
    lv_chart_set_range(satelliteBar, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
    satelliteBarSerie = lv_chart_add_series(satelliteBar, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_type(satelliteBar, LV_CHART_TYPE_BAR);
    lv_obj_set_style_pad_all(satelliteBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_gap(satelliteBar, (int)(-7 * scaleSatInfo), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(satelliteBar, (int)(2 * scaleSatInfo), 0);
    lv_obj_set_style_pad_bottom(satelliteBar, (int)(20 * scaleSatInfo), 0);
    lv_chart_set_point_count(satelliteBar, MAX_SATELLITES_IN_VIEW );
    lv_obj_add_event_cb(satelliteBar, satelliteBarDrawEvent, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_event_cb(satelliteBar, satelliteBarDrawEvent, LV_EVENT_DRAW_POST_END, NULL);
    lv_obj_add_flag(satelliteBar, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

    infoGrid = lv_obj_create(satContainer);
    lv_obj_set_width(infoGrid, TFT_WIDTH);
    lv_obj_set_flex_align(infoGrid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(infoGrid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(infoGrid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_add_style(infoGrid, &styleTransparent, LV_PART_MAIN);
    pdopLabel = lv_label_create(infoGrid);
    hdopLabel = lv_label_create(infoGrid);
    vdopLabel = lv_label_create(infoGrid);
    altLabel = lv_label_create(infoGrid);
    lv_obj_t *labels[] = {pdopLabel, hdopLabel, vdopLabel, altLabel};
    const char *texts[] = {"PDOP: %.1f", "HDOP: %.1f", "VDOP: %.1f", "ALT: %4dm."};
    for (int i = 0; i < 4; i++)
    {
        lv_obj_set_style_text_font(labels[i], fontDefault, 0);
        lv_label_set_text_fmt(labels[i], texts[i], 0);
    }
    lv_subject_add_observer_obj(&subject_pdop, dop_observer_cb, pdopLabel, (void *)"PDOP");
    lv_subject_add_observer_obj(&subject_hdop, dop_observer_cb, hdopLabel, (void *)"HDOP");
    lv_subject_add_observer_obj(&subject_vdop, dop_observer_cb, vdopLabel, (void *)"VDOP");
    lv_subject_add_observer_obj(&subject_altitude, alt_sat_observer_cb, altLabel, NULL);
    lv_subject_add_observer_obj(&subject_sats_data_trigger, sats_data_observer_cb, infoGrid, NULL);
    lv_obj_set_height(infoGrid, 40 * scale);

#ifdef TDECK_ESP32S3
    lv_obj_add_event_cb(satelliteBar, constSatEvent, LV_EVENT_LONG_PRESSED, NULL);
    constMsg = lv_msgbox_create(screen);
    lv_obj_set_size(constMsg, 180, 185);
    lv_obj_set_align(constMsg, LV_ALIGN_CENTER);
    lv_obj_add_flag(constMsg, LV_OBJ_FLAG_HIDDEN); 
    lv_obj_add_event_cb(constMsg, closeConstSatEvent, LV_EVENT_LONG_PRESSED, NULL);
#endif

#ifdef BOARD_HAS_PSRAM
    #ifndef TDECK_ESP32S3
        createSatRadar(satContainer);
    #endif
#endif
}
/**
 * @brief Draw Satellite SNR Bars
 *
 * @details Updates the SNR (Signal-to-Noise Ratio) bar chart with current satellite data.
 */
void drawSatSNR()
{
    for (int i = 0; i < MAX_SATELLITES_IN_VIEW ; i++)
        lv_chart_set_value_by_id(satelliteBar, satelliteBarSerie, i, LV_CHART_POINT_NONE);
    for (int i = 0; i < gps.gpsData.satInView && i < MAX_SATELLITES_IN_VIEW; ++i)
    {
        if (gps.satTracker[i].snr > 0)
            lv_chart_set_value_by_id(satelliteBar, satelliteBarSerie, i, gps.satTracker[i].snr);
        else
            lv_chart_set_value_by_id(satelliteBar, satelliteBarSerie, i, LV_CHART_POINT_NONE);
    }
    lv_chart_refresh(satelliteBar);
}