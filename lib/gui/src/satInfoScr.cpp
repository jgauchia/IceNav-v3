/**
 * @file satInfoScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Satellite info screen 
 * @version 0.2.0
 * @date 2025-04
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

Gps gps;

/**
 * @brief Draw Text on SNR Chart
 *
 * @param text -> Text
 * @param layer 
 * @param p
 * @param coords
 * @param color
 * @param font
 * @param offset
 */
void drawTextOnLayer(const char * text, lv_layer_t * layer, lv_point_t * p, lv_area_t * coords, lv_color_t color, const void * font, int16_t offset)
{
  lv_draw_rect_dsc_t draw_rect_dsc;
  lv_draw_rect_dsc_init(&draw_rect_dsc);

  draw_rect_dsc.bg_opa = LV_OPA_TRANSP;
  draw_rect_dsc.radius = 0;
  draw_rect_dsc.bg_image_symbol_font = font;
  draw_rect_dsc.bg_image_src = text;
  draw_rect_dsc.bg_image_recolor = color;

  lv_area_t a;
  a.x1 = coords->x1 + p->x - 10;
  a.x2 = coords->x1 + p->x + 10;
  a.y1 = coords->y1 + p->y + 10 - offset;
  a.y2 = a.y1 - 20;

  lv_draw_rect(layer, &draw_rect_dsc, &a);
}

/**
 * @brief SNR Bar draw event
 *
 * @param event
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
      //Change color/border of bar depending on GNSS and if SV is in use
      if (lv_draw_task_get_type(drawTask) == LV_DRAW_TASK_TYPE_FILL) 
      {
        lv_draw_fill_dsc_t * fill_dsc = lv_draw_task_get_fill_dsc(drawTask);
        if(fill_dsc) 
        {
        if ( strcmp(gps.satTracker[dscId].talker_id,"GP") == 0 )
          fill_dsc->color = gps.satTracker[dscId].active == true ? GP_INACTIVE_COLOR : GP_ACTIVE_COLOR;
        if ( strcmp(gps.satTracker[dscId].talker_id,"GL") == 0 )
          fill_dsc->color = gps.satTracker[dscId].active == true ? GL_INACTIVE_COLOR : GL_ACTIVE_COLOR;
        if ( strcmp(gps.satTracker[dscId].talker_id,"BD") == 0 )
          fill_dsc->color = gps.satTracker[dscId].active == true ? BD_INACTIVE_COLOR : BD_ACTIVE_COLOR;
        }
      }
    }
  }

  if (e == LV_EVENT_DRAW_POST_END) 
  {
    lv_layer_t * layer = lv_event_get_layer(event);
    char buf[16];
    
    for (uint16_t i = 0; i < gps.gpsData.satInView; i++) 
    {
      lv_area_t chartObjCoords;
      lv_obj_get_coords(obj, &chartObjCoords);
      lv_point_t p;
      lv_chart_get_point_pos_by_id(obj, lv_chart_get_series_next(obj, NULL), i, &p);

      //Draw signal at top of bar
      if (gps.satTracker[i].snr > 0)
      {
        lv_snprintf(buf, sizeof(buf), LV_SYMBOL_DUMMY"%d", gps.satTracker[i].snr);
        drawTextOnLayer(buf, layer, &p, &chartObjCoords, lv_color_white(), fontSmall, 15);
      }

      //Draw Satellite ID
      lv_snprintf(buf, sizeof(buf), LV_SYMBOL_DUMMY"%d", gps.satTracker[i].satNum);
      drawTextOnLayer(buf, layer, &p, &chartObjCoords, lv_color_white(), fontSmall, (chartObjCoords.y1 + p.y) - chartObjCoords.y2 + 10);
    }
  } 
}

/**
 * @brief SNR Long press event for show constellation Map (only for T-DECK)
 *
 * @param event
 */
void constSatEvent(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_LONG_PRESSED)
    lv_obj_clear_flag(constMsg,LV_OBJ_FLAG_HIDDEN); 
}

/**
 * @brief Event for hide constellation Map (only for T-DECK)
 *
 * @param event
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
 * @param screen 
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
 * @param screen 
 */
#ifndef TDECK_ESP32S3
void satelliteScr(_lv_obj_t *screen)
{
  lv_obj_t *infoGrid = lv_obj_create(screen);
  lv_obj_set_width(infoGrid,TFT_WIDTH);
  lv_obj_set_height(infoGrid,35);
  lv_obj_set_flex_align(infoGrid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(infoGrid, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(infoGrid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_pos(infoGrid,0,190);

  static lv_style_t styleGrid;
  lv_style_init(&styleGrid);
  lv_style_set_bg_opa(&styleGrid, LV_OPA_0);
  lv_style_set_border_opa(&styleGrid, LV_OPA_0);
  lv_obj_add_style(infoGrid, &styleGrid, LV_PART_MAIN);
        
  pdopLabel = lv_label_create(infoGrid);
  lv_obj_set_style_text_font(pdopLabel, fontDefault, 0);
  lv_label_set_text_fmt(pdopLabel, "PDOP: %.1f", 0);
  
  hdopLabel = lv_label_create(infoGrid);
  lv_obj_set_style_text_font(hdopLabel, fontDefault, 0);
  lv_label_set_text_fmt(hdopLabel, "HDOP: %.1f", 0);
  
  vdopLabel = lv_label_create(infoGrid);
  lv_obj_set_style_text_font(vdopLabel, fontDefault, 0);
  lv_label_set_text_fmt(vdopLabel, "VDOP: %.1f", 0);
  
  altLabel = lv_label_create(infoGrid);
  lv_obj_set_style_text_font(altLabel, fontDefault, 0);
  lv_label_set_text_fmt(altLabel, "ALT: %4dm.", 0); 
  
  lv_obj_t * barCont = lv_obj_create(screen);
  lv_obj_set_size(barCont, TFT_WIDTH, 180);
  lv_obj_set_pos(barCont, 0, 5);
  lv_obj_t * wrapper = lv_obj_create(barCont);
  lv_obj_remove_style_all(wrapper);
  lv_obj_set_size(wrapper, TFT_WIDTH * 2, 120);

  lv_obj_t * gnssLabel = lv_label_create(barCont);
  lv_obj_set_style_text_font(gnssLabel, fontSatInfo, 0);
  lv_obj_set_pos(gnssLabel, 0, 127);
  lv_obj_set_width(gnssLabel,90);
  lv_obj_set_style_bg_color(gnssLabel, GP_ACTIVE_COLOR, 0);
  lv_obj_set_style_bg_opa(gnssLabel, LV_OPA_100, 0);
  lv_obj_set_style_border_color(gnssLabel, GP_INACTIVE_COLOR, 0);
  lv_obj_set_style_border_width(gnssLabel, 1, 0);
  lv_obj_set_style_border_opa(gnssLabel, LV_OPA_100, 0);
  lv_label_set_text(gnssLabel, "GPS");
  lv_obj_set_style_text_align(gnssLabel, LV_TEXT_ALIGN_CENTER, 0);

  gnssLabel = lv_label_create(barCont);
  lv_obj_set_style_text_font(gnssLabel, fontSatInfo, 0);
  lv_obj_set_pos(gnssLabel, 95, 127);
  lv_obj_set_width(gnssLabel,90);
  lv_obj_set_style_bg_color(gnssLabel, GL_ACTIVE_COLOR, 0);
  lv_obj_set_style_bg_opa(gnssLabel, LV_OPA_100, 0);
  lv_obj_set_style_border_color(gnssLabel, GL_INACTIVE_COLOR, 0);
  lv_obj_set_style_border_width(gnssLabel, 1, 0);
  lv_obj_set_style_border_opa(gnssLabel, LV_OPA_100, 0);
  lv_label_set_text(gnssLabel, "GLONASS");
  lv_obj_set_style_text_align(gnssLabel, LV_TEXT_ALIGN_CENTER, 0);

  gnssLabel = lv_label_create(barCont);
  lv_obj_set_style_text_font(gnssLabel, fontSatInfo, 0);
  lv_obj_set_pos(gnssLabel, 190, 127);
  lv_obj_set_width(gnssLabel,90);
  lv_obj_set_style_bg_color(gnssLabel, BD_ACTIVE_COLOR, 0);
  lv_obj_set_style_bg_opa(gnssLabel, LV_OPA_100, 0);
  lv_obj_set_style_border_color(gnssLabel, BD_INACTIVE_COLOR, 0);
  lv_obj_set_style_border_width(gnssLabel, 1, 0);
  lv_obj_set_style_border_opa(gnssLabel, LV_OPA_100, 0);
  lv_label_set_text(gnssLabel, "BEIDOU");
  lv_obj_set_style_text_align(gnssLabel, LV_TEXT_ALIGN_CENTER, 0);

  satelliteBar = lv_chart_create(wrapper);
  lv_obj_set_size(satelliteBar, TFT_WIDTH * 2, 120);
  lv_chart_set_div_line_count(satelliteBar, 10, 0);
  lv_chart_set_range(satelliteBar, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
  satelliteBarSerie = lv_chart_add_series(satelliteBar, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_set_type(satelliteBar, LV_CHART_TYPE_BAR);
  lv_obj_set_style_pad_all(satelliteBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_gap(satelliteBar, -7, LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_column(satelliteBar, 2, 0);
  lv_chart_set_point_count(satelliteBar, MAX_SATELLLITES_IN_VIEW );
  lv_obj_add_event_cb(satelliteBar, satelliteBarDrawEvent, LV_EVENT_DRAW_TASK_ADDED, NULL);
  lv_obj_add_event_cb(satelliteBar, satelliteBarDrawEvent, LV_EVENT_DRAW_POST_END, NULL);
  lv_obj_add_flag(satelliteBar, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
}
#endif

#ifdef TDECK_ESP32S3
void satelliteScr(_lv_obj_t *screen)
{
  lv_obj_t *infoGrid = lv_obj_create(screen);
  lv_obj_set_width(infoGrid,TFT_WIDTH);
  lv_obj_set_height(infoGrid,35);
  lv_obj_set_flex_align(infoGrid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(infoGrid, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(infoGrid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_pos(infoGrid,0,150);

  static lv_style_t styleGrid;
  lv_style_init(&styleGrid);
  lv_style_set_bg_opa(&styleGrid, LV_OPA_0);
  lv_style_set_border_opa(&styleGrid, LV_OPA_0);
  lv_obj_add_style(infoGrid, &styleGrid, LV_PART_MAIN);
        
  pdopLabel = lv_label_create(infoGrid);
  lv_obj_set_style_text_font(pdopLabel, fontDefault, 0);
  lv_label_set_text_fmt(pdopLabel, "PDOP: %.1f", 0);
  
  hdopLabel = lv_label_create(infoGrid);
  lv_obj_set_style_text_font(hdopLabel, fontDefault, 0);
  lv_label_set_text_fmt(hdopLabel, "HDOP: %.1f", 0);
  
  vdopLabel = lv_label_create(infoGrid);
  lv_obj_set_style_text_font(vdopLabel, fontDefault, 0);
  lv_label_set_text_fmt(vdopLabel, "VDOP: %.1f", 0);
  
  altLabel = lv_label_create(infoGrid);
  lv_obj_set_style_text_font(altLabel, fontDefault, 0);
  lv_label_set_text_fmt(altLabel, "ALT: %4dm.", 0); 
  
  lv_obj_t * barCont = lv_obj_create(screen);
  lv_obj_set_size(barCont, TFT_WIDTH, 145);
  lv_obj_set_pos(barCont, 0, 5);
  
  lv_obj_t * wrapper = lv_obj_create(barCont);
  lv_obj_remove_style_all(wrapper);
  lv_obj_set_size(wrapper, TFT_WIDTH * 2, 100);

  lv_obj_t * gnssLabel = lv_label_create(barCont);
  lv_obj_set_style_text_font(gnssLabel, fontSatInfo, 0);
  lv_obj_set_pos(gnssLabel, 0, 102);
  lv_obj_set_width(gnssLabel,90);
  lv_obj_set_style_bg_color(gnssLabel, GP_ACTIVE_COLOR, 0);
  lv_obj_set_style_bg_opa(gnssLabel, LV_OPA_100, 0);
  lv_obj_set_style_border_color(gnssLabel, GP_INACTIVE_COLOR, 0);
  lv_obj_set_style_border_width(gnssLabel, 1, 0);
  lv_obj_set_style_border_opa(gnssLabel, LV_OPA_100, 0);
  lv_label_set_text(gnssLabel, "GPS");
  lv_obj_set_style_text_align(gnssLabel, LV_TEXT_ALIGN_CENTER, 0);

  gnssLabel = lv_label_create(barCont);
  lv_obj_set_style_text_font(gnssLabel, fontSatInfo, 0);
  lv_obj_set_pos(gnssLabel, 95, 102);
  lv_obj_set_width(gnssLabel,90);
  lv_obj_set_style_bg_color(gnssLabel, GL_ACTIVE_COLOR, 0);
  lv_obj_set_style_bg_opa(gnssLabel, LV_OPA_100, 0);
  lv_obj_set_style_border_color(gnssLabel, GL_INACTIVE_COLOR, 0);
  lv_obj_set_style_border_width(gnssLabel, 1, 0);
  lv_obj_set_style_border_opa(gnssLabel, LV_OPA_100, 0);
  lv_label_set_text(gnssLabel, "GLONASS");
  lv_obj_set_style_text_align(gnssLabel, LV_TEXT_ALIGN_CENTER, 0);

  gnssLabel = lv_label_create(barCont);
  lv_obj_set_style_text_font(gnssLabel, fontSatInfo, 0);
  lv_obj_set_pos(gnssLabel, 190, 102);
  lv_obj_set_width(gnssLabel,90);
  lv_obj_set_style_bg_color(gnssLabel, BD_ACTIVE_COLOR, 0);
  lv_obj_set_style_bg_opa(gnssLabel, LV_OPA_100, 0);
  lv_obj_set_style_border_color(gnssLabel, BD_INACTIVE_COLOR, 0);
  lv_obj_set_style_border_width(gnssLabel, 1, 0);
  lv_obj_set_style_border_opa(gnssLabel, LV_OPA_100, 0);
  lv_label_set_text(gnssLabel, "BEIDOU");
  lv_obj_set_style_text_align(gnssLabel, LV_TEXT_ALIGN_CENTER, 0);

  satelliteBar = lv_chart_create(wrapper);
  lv_obj_set_size(satelliteBar, TFT_WIDTH * 2, 100);
  lv_chart_set_div_line_count(satelliteBar, 10, 0);
  lv_chart_set_range(satelliteBar, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
  satelliteBarSerie = lv_chart_add_series(satelliteBar, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_set_type(satelliteBar, LV_CHART_TYPE_BAR);
  lv_obj_set_style_pad_all(satelliteBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_gap(satelliteBar, -7, LV_PART_ITEMS | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_column(satelliteBar, 2, 0);
  lv_chart_set_point_count(satelliteBar, MAX_SATELLLITES_IN_VIEW );
  lv_obj_add_event_cb(satelliteBar, satelliteBarDrawEvent, LV_EVENT_DRAW_TASK_ADDED, NULL);
  lv_obj_add_event_cb(satelliteBar, satelliteBarDrawEvent, LV_EVENT_DRAW_POST_END, NULL);
  lv_obj_add_flag(satelliteBar, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
  lv_obj_add_event_cb(satelliteBar, constSatEvent, LV_EVENT_LONG_PRESSED, NULL);

  constMsg = lv_msgbox_create(screen);
  lv_obj_set_size(constMsg, 180, 185);
  lv_obj_set_align(constMsg,LV_ALIGN_CENTER);
  lv_obj_add_flag(constMsg,LV_OBJ_FLAG_HIDDEN); 
  lv_obj_add_event_cb(constMsg, closeConstSatEvent, LV_EVENT_LONG_PRESSED, NULL);
}
#endif

/**
 * @brief Draw Satellite SNR Bars
 *
 *
 */
void drawSatSNR()
{
  for (int i = 0; i < MAX_SATELLLITES_IN_VIEW ; i++)
  {
    lv_chart_set_value_by_id(satelliteBar, satelliteBarSerie, i, LV_CHART_POINT_NONE);
  }

  for (int i = 0; i < gps.gpsData.satInView; ++i)
  {
    lv_chart_set_value_by_id(satelliteBar, satelliteBarSerie, i, gps.satTracker[i].snr);
  }

  lv_chart_refresh(satelliteBar);
}

/**
 * @brief Draw Satellite Constellation
 *
 *
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

  // Finish Canvas Draw 
  lv_canvas_finish_layer(constCanvas, &canvasLayer); 
}

/**
 * @brief Draw Satellite Position in Constellation
 *
 *
 */
void drawSatSky()
{
  lv_canvas_fill_bg(constCanvas, lv_color_black(), LV_OPA_100);
  drawSatConst();

  lv_layer_t satPosLayer;
  lv_canvas_init_layer(constCanvas, &satPosLayer);

  // Draw Satellite
  lv_draw_arc_dsc_t dscSat;
  lv_draw_arc_dsc_init(&dscSat);
  dscSat.width = 8;
  dscSat.start_angle = 0;
  dscSat.end_angle = 360;
  dscSat.radius = 8;
  dscSat.opa = LV_OPA_70;

  for (int i = 0; i < gps.gpsData.satInView; i++)
  {
    if ( strcmp(gps.satTracker[i].talker_id,"GP") == 0 )
      dscSat.color = gps.satTracker[i].active == true ? GP_INACTIVE_COLOR : GP_ACTIVE_COLOR;
    if ( strcmp(gps.satTracker[i].talker_id,"GL") == 0 )
      dscSat.color = gps.satTracker[i].active == true ? GL_INACTIVE_COLOR : GL_ACTIVE_COLOR;
    if ( strcmp(gps.satTracker[i].talker_id,"BD") == 0 )
      dscSat.color = gps.satTracker[i].active == true ? BD_INACTIVE_COLOR : BD_ACTIVE_COLOR;
    dscSat.center.x = gps.satTracker[i].posX;
    dscSat.center.y = gps.satTracker[i].posY;
    lv_draw_arc(&satPosLayer, &dscSat);

    // Draw Satellite Number
    char buf[16];
    lv_draw_rect_dsc_t draw_rect_dsc;
    lv_draw_rect_dsc_init(&draw_rect_dsc);

    lv_snprintf(buf, sizeof(buf), LV_SYMBOL_DUMMY"%d", gps.satTracker[i].satNum);

    draw_rect_dsc.bg_opa = LV_OPA_TRANSP;
    draw_rect_dsc.radius = 0;
    draw_rect_dsc.bg_image_symbol_font = &lv_font_montserrat_8;
    draw_rect_dsc.bg_image_src = buf;
    draw_rect_dsc.bg_image_recolor = lv_color_white();

    lv_area_t a;
    a.x1 = gps.satTracker[i].posX-8;
    a.x2 = gps.satTracker[i].posX+8;
    a.y1 = gps.satTracker[i].posY-5;
    a.y2 = gps.satTracker[i].posY+4;

    lv_draw_rect(&satPosLayer, &draw_rect_dsc, &a);

    // Finish Canvas Draw 
    lv_canvas_finish_layer(constCanvas, &satPosLayer); 
  }
}