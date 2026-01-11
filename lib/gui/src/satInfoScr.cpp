/**
 * @file satInfoScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Satellite info screen 
 * @version 0.2.4
 * @date 2025-12
 */
#include "satInfoScr.hpp"
lv_obj_t *pdopLabel;
lv_obj_t *hdopLabel;
lv_obj_t *vdopLabel;
lv_obj_t *altLabel;
lv_obj_t *constCanvas;
lv_layer_t canvasLayer;
lv_layer_t satLayer;
lv_obj_t *satelliteBar;               
lv_chart_series_t *satelliteBarSerie; 
lv_obj_t *constMsg;
extern Gps gps;
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
        if(base_dsc->part == LV_PART_ITEMS)
        {
            uint16_t dscId = base_dsc->id2;
            if (lv_draw_task_get_type(drawTask) == LV_DRAW_TASK_TYPE_FILL) 
            {
                lv_draw_fill_dsc_t * fill_dsc = lv_draw_task_get_fill_dsc(drawTask);
                if(fill_dsc) 
                {
                    if ( strcmp(gps.satTracker[dscId].talker_id,"GP") == 0 )
                        fill_dsc->color = gps.satTracker[dscId].active ? GP_ACTIVE_COLOR : GP_INACTIVE_COLOR;
                    if ( strcmp(gps.satTracker[dscId].talker_id,"GL") == 0 )
                        fill_dsc->color = gps.satTracker[dscId].active ? GL_ACTIVE_COLOR : GL_INACTIVE_COLOR;
                    if ( strcmp(gps.satTracker[dscId].talker_id,"BD") == 0 )
                        fill_dsc->color = gps.satTracker[dscId].active ? BD_ACTIVE_COLOR : BD_INACTIVE_COLOR;
                }
            }
        }
    }
    if (e == LV_EVENT_DRAW_POST_END) 
    {
        lv_layer_t * layer = lv_event_get_layer(event);
        static char label_bufs[MAX_SATELLLITES_IN_VIEW][2][8];
        lv_area_t chartObjCoords;
        lv_obj_get_coords(obj, &chartObjCoords);
        int32_t chartWidth = lv_obj_get_width(obj);
        float barWidth = (float)chartWidth / MAX_SATELLLITES_IN_VIEW;
        for (uint16_t i = 0; i < gps.gpsData.satInView && i < MAX_SATELLLITES_IN_VIEW; i++) 
        {
            int32_t centerX = chartObjCoords.x1 + (int32_t)(i * barWidth + barWidth/2);
            lv_point_t p;
            lv_chart_get_point_pos_by_id(obj, lv_chart_get_series_next(obj, NULL), i, &p);
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
                a.x1 = centerX - 20;
                a.x2 = centerX + 20;
                a.y1 = chartObjCoords.y1 + p.y - 15;
                a.y2 = a.y1 + 15;
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
            aId.x1 = centerX - 20;
            aId.x2 = centerX + 20;
            aId.y1 = chartObjCoords.y2 - 16;
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
 * @brief Create Canvas for Satellite Constellation
 *
 * @details Initializes and creates the canvas object for rendering the satellite constellation on the specified screen,
 * 			allocates the required buffer, and sets up the drawing layer.
 *
 * @param screen Pointer to the LVGL screen object where the canvas will be created.
 */
void createConstCanvas(_lv_obj_t *screen)
{
    static lv_color_t *cbuf = (lv_color_t*)heap_caps_aligned_alloc(16, (canvasSize*canvasSize*sizeof(lv_color_t)), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);;
    constCanvas = lv_canvas_create(screen);
    lv_canvas_set_buffer(constCanvas, cbuf, canvasSize, canvasSize, LV_COLOR_FORMAT_RGB565);
    lv_canvas_fill_bg(constCanvas, lv_color_black(), LV_OPA_100);
    lv_canvas_init_layer(constCanvas, &canvasLayer);
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
    lv_obj_t *infoGrid = lv_obj_create(screen);
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
    for(int i=0; i<4; i++) {
        lv_obj_set_style_text_font(labels[i], fontDefault, 0);
        lv_label_set_text_fmt(labels[i], texts[i], 0);
    }
    lv_obj_t * barCont = lv_obj_create(screen);
    lv_obj_set_pos(barCont, 0, 5);
#ifdef TDECK_ESP32S3
    lv_obj_set_size(barCont, TFT_WIDTH, 145);
#else
    lv_obj_set_size(barCont, TFT_WIDTH, 180);
#endif
    uint16_t barHeight = 120;
#ifdef TDECK_ESP32S3
    barHeight = 100;
#endif
    lv_obj_t * wrapper = lv_obj_create(barCont);
    lv_obj_remove_style_all(wrapper);
    lv_obj_set_size(wrapper, TFT_WIDTH * 2, barHeight);
    const char* gnssNames[] = {"GPS", "GLONASS", "BEIDOU"};
    lv_color_t activeColors[] = {GP_ACTIVE_COLOR, GL_ACTIVE_COLOR, BD_ACTIVE_COLOR};
    lv_color_t inactiveColors[] = {GP_INACTIVE_COLOR, GL_INACTIVE_COLOR, BD_INACTIVE_COLOR};
    for(int i=0; i<3; i++) {
        lv_obj_t *gnssLabel = lv_label_create(barCont);
        lv_obj_set_style_text_font(gnssLabel, fontSatInfo, 0);
        lv_obj_set_width(gnssLabel, 90);
        lv_obj_set_style_bg_color(gnssLabel, activeColors[i], 0);
        lv_obj_set_style_bg_opa(gnssLabel, LV_OPA_100, 0);
        lv_obj_set_style_border_color(gnssLabel, inactiveColors[i], 0);
        lv_obj_set_style_border_width(gnssLabel, 1, 0);
        lv_obj_set_style_border_opa(gnssLabel, LV_OPA_100, 0);
        lv_label_set_text(gnssLabel, gnssNames[i]);
        lv_obj_set_style_text_align(gnssLabel, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_pos(gnssLabel, i * 95, barHeight + 7);
    }
    satelliteBar = lv_chart_create(wrapper);
    lv_obj_set_size(satelliteBar, TFT_WIDTH * 2, barHeight);
    lv_chart_set_div_line_count(satelliteBar, 10, 0);
    lv_chart_set_range(satelliteBar, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
    satelliteBarSerie = lv_chart_add_series(satelliteBar, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_type(satelliteBar, LV_CHART_TYPE_BAR);
    lv_obj_set_style_pad_all(satelliteBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_gap(satelliteBar, -7, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(satelliteBar, 2, 0);
    lv_obj_set_style_pad_bottom(satelliteBar, 20, 0);
    lv_chart_set_point_count(satelliteBar, MAX_SATELLLITES_IN_VIEW );
    lv_obj_add_event_cb(satelliteBar, satelliteBarDrawEvent, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_event_cb(satelliteBar, satelliteBarDrawEvent, LV_EVENT_DRAW_POST_END, NULL);
    lv_obj_add_flag(satelliteBar, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
#ifdef TDECK_ESP32S3
    lv_obj_set_height(infoGrid, 35);
    lv_obj_set_pos(infoGrid, 0, 150);
    lv_obj_add_event_cb(satelliteBar, constSatEvent, LV_EVENT_LONG_PRESSED, NULL);
    constMsg = lv_msgbox_create(screen);
    lv_obj_set_size(constMsg, 180, 185);
    lv_obj_set_align(constMsg, LV_ALIGN_CENTER);
    lv_obj_add_flag(constMsg, LV_OBJ_FLAG_HIDDEN); 
    lv_obj_add_event_cb(constMsg, closeConstSatEvent, LV_EVENT_LONG_PRESSED, NULL);
#else
    lv_obj_set_height(infoGrid, 40 * scale);
    lv_obj_set_pos(infoGrid, 0, 190);
#endif
}
/**
 * @brief Draw Satellite SNR Bars
 *
 * @details Updates the SNR (Signal-to-Noise Ratio) bar chart with current satellite data.
 */
void drawSatSNR()
{
    for (int i = 0; i < MAX_SATELLLITES_IN_VIEW ; i++)
    {
        lv_chart_set_value_by_id(satelliteBar, satelliteBarSerie, i, LV_CHART_POINT_NONE);
    }
    for (int i = 0; i < gps.gpsData.satInView; ++i)
    {
        if (gps.satTracker[i].snr > 0)
            lv_chart_set_value_by_id(satelliteBar, satelliteBarSerie, i, gps.satTracker[i].snr);
        else
            lv_chart_set_value_by_id(satelliteBar, satelliteBarSerie, i, LV_CHART_POINT_NONE);
    }
    lv_chart_refresh(satelliteBar);
}
/**
 * @brief Draw Satellite Constellation
 *
 * @details Draws the satellite constellation grid on the canvas, including concentric circles, cross lines, and cardinal direction labels.
 */
void drawSatConst()
{
    // Draw Circles
    lv_draw_arc_dsc_t dscArc;
    lv_draw_arc_dsc_init(&dscArc);
    dscArc.color = CONSTEL_COLOR;
    dscArc.width = 2;
    dscArc.center.x = canvasCenter_X;
    dscArc.center.y = canvasCenter_Y;
    dscArc.start_angle = 0;
    dscArc.end_angle = 360;
    dscArc.radius = canvasRadius;
    lv_draw_arc(&canvasLayer, &dscArc);
    dscArc.radius = ( canvasRadius * 2 ) / 3;
    lv_draw_arc(&canvasLayer, &dscArc);
    dscArc.radius = canvasRadius / 3 ;
    lv_draw_arc(&canvasLayer, &dscArc);
    // Draw Lines
    lv_draw_line_dsc_t dscLine;
    lv_draw_line_dsc_init(&dscLine);
    dscLine.color = CONSTEL_COLOR;
    dscLine.width = 2;
    dscLine.round_end = 1;
    dscLine.round_start = 1;
    dscLine.p1.x = canvasCenter_X;
    dscLine.p1.y = canvasOffset;
    dscLine.p2.x = canvasCenter_X;
    dscLine.p2.y = canvasSize-canvasOffset;
    lv_draw_line(&canvasLayer, &dscLine);
    dscLine.p1.x = canvasOffset;
    dscLine.p1.y = canvasCenter_Y;
    dscLine.p2.x = canvasSize-canvasOffset;
    dscLine.p2.y = canvasCenter_Y;
    lv_draw_line(&canvasLayer, &dscLine);
    // Draw Text Coordinates
    lv_draw_label_dsc_t dscLabel;
    lv_draw_label_dsc_init(&dscLabel);
    dscLabel.color = CONSTEL_COLOR;
    dscLabel.opa = LV_OPA_100;
    dscLabel.font = &lv_font_montserrat_12;
    dscLabel.text = "N";
    lv_area_t labelPos = {canvasCenter_X-5, 0, canvasCenter_X+5, 0};
    lv_draw_label(&canvasLayer, &dscLabel, &labelPos);
    dscLabel.text = "S";
    labelPos = {canvasCenter_X-4, canvasSize-15, canvasCenter_X+4, canvasSize};
    lv_draw_label(&canvasLayer, &dscLabel, &labelPos);
    dscLabel.text = "E";
    labelPos = {canvasSize-12, canvasCenter_Y-7 , canvasSize, canvasCenter_Y+7};
    lv_draw_label(&canvasLayer, &dscLabel, &labelPos);
    dscLabel.text = "W";
    labelPos = {0, canvasCenter_Y-7, canvasSize - 10, canvasCenter_Y+7};
    lv_draw_label(&canvasLayer, &dscLabel, &labelPos);
    lv_canvas_finish_layer(constCanvas, &canvasLayer); 
}
/**
 * @brief Draw Satellite Position in Constellation
 *
 * @details Draws the position of each satellite within the constellation canvas, including their colored circles (based on constellation and activity)
 * 			and overlays their satellite numbers at their respective positions.
 */
void drawSatSky()
{
    lv_canvas_fill_bg(constCanvas, lv_color_black(), LV_OPA_100);
    drawSatConst();
    lv_draw_arc_dsc_t dscSat;
    lv_draw_arc_dsc_init(&dscSat);
    dscSat.width = 8;
    dscSat.start_angle = 0;
    dscSat.end_angle = 360;
    dscSat.radius = 8;
    dscSat.opa = LV_OPA_70;
    for (int i = 0; i < gps.gpsData.satInView; i++) 
    {
        lv_layer_t satPosLayer;
        lv_canvas_init_layer(constCanvas, &satPosLayer);
        if ( strcmp(gps.satTracker[i].talker_id,"GP") == 0 )
            dscSat.color = gps.satTracker[i].active ? GP_ACTIVE_COLOR : GP_INACTIVE_COLOR;
        if ( strcmp(gps.satTracker[i].talker_id,"GL") == 0 )
            dscSat.color = gps.satTracker[i].active ? GL_ACTIVE_COLOR : GL_INACTIVE_COLOR;
        if ( strcmp(gps.satTracker[i].talker_id,"BD") == 0 )
            dscSat.color = gps.satTracker[i].active ? BD_ACTIVE_COLOR : BD_INACTIVE_COLOR;
        dscSat.center.x = gps.satTracker[i].posX;
        dscSat.center.y = gps.satTracker[i].posY;
        lv_draw_arc(&satPosLayer, &dscSat);
        char buf[8];
        lv_snprintf(buf, sizeof(buf), "%d", gps.satTracker[i].satNum);
        lv_draw_label_dsc_t dscLabel;
        lv_draw_label_dsc_init(&dscLabel);
        dscLabel.color = lv_color_white();
        dscLabel.font = &lv_font_montserrat_8;
        dscLabel.text = buf;
        dscLabel.align = LV_TEXT_ALIGN_CENTER;
        lv_area_t a;
        a.x1 = gps.satTracker[i].posX - 12;
        a.x2 = gps.satTracker[i].posX + 12;
        a.y1 = gps.satTracker[i].posY - 6;
        a.y2 = gps.satTracker[i].posY + 6;
        lv_draw_label(&satPosLayer, &dscLabel, &a);
        lv_canvas_finish_layer(constCanvas, &satPosLayer); 
    }
}
