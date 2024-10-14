/**
 * @file satInfoScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL - Satellite info screen 
 * @version 0.1.8_Alpha
 * @date 2024-10
 */

 #include "satInfoScr.hpp"


lv_obj_t *pdopLabel;
lv_obj_t *hdopLabel;
lv_obj_t *vdopLabel;
lv_obj_t *altLabel;
lv_style_t styleRadio;
lv_style_t styleRadioChk;
uint32_t activeGnss = 0;

/**
 * @brief GNSS Selection Checkbox event
 *
 * @param event
 */
void activeGnssEvent(lv_event_t *event)
{
  uint32_t *activeId = (uint32_t *)lv_event_get_user_data(event);
  lv_obj_t *cont = (lv_obj_t *)lv_event_get_current_target(event);
  lv_obj_t *activeCheckBox = (lv_obj_t *)lv_event_get_target(event);
  lv_obj_t *oldCheckBox = lv_obj_get_child(cont, *activeId);
  
  if (activeCheckBox == cont)
    return;
  
  lv_obj_clear_state(oldCheckBox, LV_STATE_CHECKED);
  lv_obj_add_state(activeCheckBox, LV_STATE_CHECKED);
  
  clearSatInView();
  
  *activeId = lv_obj_get_index(activeCheckBox);
}

/**
 * @brief SNR Bar draw event
 *
 * @param event
 */
void satelliteBar1DrawEvent(lv_event_t * event)
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
            fill_dsc->color = lv_palette_main(LV_PALETTE_BLUE);
            //fill_dsc->width = 7;
          }
        }
        if (lv_draw_task_get_type(drawTask) == LV_DRAW_TASK_TYPE_BORDER) 
        {
          // lv_draw_border_dsc_t * border_dsc = lv_draw_task_get_border_dsc(drawTask);
          // if (border_dsc) {
          //   border_dsc->width = gnss_svInfo[dscId].bInUse == true ? 1 : 0;
          //   border_dsc->color = gnss_svInfo[dscId].bInUse == true ? lv_color_white() : lv_color_black();
        }
    }
  }

  if (e == LV_EVENT_DRAW_POST_END) 
  {

  }
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
        lv_obj_set_size(infoGrid, 90, 175);
        lv_obj_set_flex_align(infoGrid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(infoGrid, 5 * scale, 0);
        lv_obj_clear_flag(infoGrid, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(infoGrid, LV_FLEX_FLOW_COLUMN);
        static lv_style_t styleGrid;
        lv_style_init(&styleGrid);
        lv_style_set_bg_opa(&styleGrid, LV_OPA_0);
        lv_style_set_border_opa(&styleGrid, LV_OPA_0);
        lv_obj_add_style(infoGrid, &styleGrid, LV_PART_MAIN);
        lv_obj_set_y(infoGrid,0);
        
        pdopLabel = lv_label_create(infoGrid);
        lv_obj_set_style_text_font(pdopLabel, fontSatInfo, 0);
        lv_label_set_text_fmt(pdopLabel, "PDOP:\n%s", "0.0");
        
        hdopLabel = lv_label_create(infoGrid);
        lv_obj_set_style_text_font(hdopLabel, fontSatInfo, 0);
        lv_label_set_text_fmt(hdopLabel, "HDOP:\n%s", "0.0");
        
        vdopLabel = lv_label_create(infoGrid);
        lv_obj_set_style_text_font(vdopLabel, fontSatInfo, 0);
        lv_label_set_text_fmt(vdopLabel, "VDOP:\n%s", "0.0");
        
        altLabel = lv_label_create(infoGrid);
        lv_obj_set_style_text_font(altLabel, fontSatInfo, 0);
        lv_label_set_text_fmt(altLabel, "ALT:\n%4dm.", 0); 
        
        satelliteBar1 = lv_chart_create(screen);
        lv_obj_set_size(satelliteBar1, TFT_WIDTH, 55 * scale);
        lv_chart_set_div_line_count(satelliteBar1, 6, 0);
        lv_chart_set_range(satelliteBar1, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
        satelliteBarSerie1 = lv_chart_add_series(satelliteBar1, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
        lv_chart_set_type(satelliteBar1, LV_CHART_TYPE_BAR);
        lv_obj_set_style_pad_all(satelliteBar1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_column(satelliteBar1, 1, 0);
        lv_chart_set_point_count(satelliteBar1, (MAX_SATELLLITES_IN_VIEW / 2));
        lv_obj_set_pos(satelliteBar1, 0, 175 * scale);
        lv_obj_add_event_cb(satelliteBar1, satelliteBar1DrawEvent, LV_EVENT_DRAW_TASK_ADDED, NULL);
        lv_obj_add_event_cb(satelliteBar1, satelliteBar1DrawEvent, LV_EVENT_DRAW_POST_END, NULL);
        lv_obj_add_flag(satelliteBar1, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

        


        satelliteBar2 = lv_chart_create(screen);
        lv_obj_set_size(satelliteBar2, TFT_WIDTH, 55 * scale);
        lv_chart_set_div_line_count(satelliteBar2, 6, 0);
        lv_chart_set_range(satelliteBar2, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
        satelliteBarSerie2 = lv_chart_add_series(satelliteBar2, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
        lv_chart_set_type(satelliteBar2, LV_CHART_TYPE_BAR);
        lv_chart_set_point_count(satelliteBar2, (MAX_SATELLLITES_IN_VIEW / 2));
        lv_obj_set_pos(satelliteBar2, 0, 260 * scale);
        
        #ifdef LARGE_SCREEN

        #ifdef AT6558D_GPS
        lv_style_init(&styleRadio);
        lv_style_set_radius(&styleRadio, LV_RADIUS_CIRCLE);
        
        lv_style_init(&styleRadioChk);
        lv_style_set_bg_image_src(&styleRadioChk, NULL);
        
        lv_obj_t *gnssSel = lv_obj_create(screen);
        lv_obj_set_flex_flow(gnssSel, LV_FLEX_FLOW_ROW);
        lv_obj_set_size(gnssSel, TFT_WIDTH, 50);
        lv_obj_set_pos(gnssSel, 0, 330);
        static lv_style_t styleSel;
        lv_style_init(&styleSel);
        lv_style_set_bg_opa(&styleSel, LV_OPA_0);
        lv_style_set_border_opa(&styleSel, LV_OPA_0);
        lv_obj_add_style(gnssSel, &styleSel, LV_PART_MAIN);
        
        lv_obj_t *gps = lv_checkbox_create(gnssSel);
        lv_checkbox_set_text_static(gps, "GPS     ");
        lv_obj_add_flag(gps, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_style(gps, &styleRadio, LV_PART_INDICATOR);
        lv_obj_add_style(gps, &styleRadioChk, LV_PART_INDICATOR | LV_STATE_CHECKED);
        
        lv_obj_t *glonass = lv_checkbox_create(gnssSel);
        lv_checkbox_set_text_static(glonass, "GLONASS  ");
        lv_obj_add_flag(glonass, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_style(glonass, &styleRadio, LV_PART_INDICATOR);
        lv_obj_add_style(glonass, &styleRadioChk, LV_PART_INDICATOR | LV_STATE_CHECKED);
        
        lv_obj_t *beidou = lv_checkbox_create(gnssSel);
        lv_checkbox_set_text_static(beidou, "BEIDOU");
        lv_obj_add_flag(beidou, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_style(beidou, &styleRadio, LV_PART_INDICATOR);
        lv_obj_add_style(beidou, &styleRadioChk, LV_PART_INDICATOR | LV_STATE_CHECKED);
        
        lv_obj_add_state(lv_obj_get_child(gnssSel, 0), LV_STATE_CHECKED);
        
        // GNSS Selection Event
        lv_obj_add_event_cb(gnssSel, activeGnssEvent, LV_EVENT_CLICKED, &activeGnss);
        #endif
        
        #endif
    }
#endif

#ifdef TDECK_ESP32S3
    void satelliteScr(_lv_obj_t *screen)
    {
        lv_obj_t *infoGrid = lv_obj_create(screen);
        lv_obj_set_size(infoGrid, 90, 155);
        lv_obj_set_flex_align(infoGrid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(infoGrid, 5 * scale, 0);
        lv_obj_clear_flag(infoGrid, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(infoGrid, LV_FLEX_FLOW_COLUMN);
        static lv_style_t styleGrid;
        lv_style_init(&styleGrid);
        lv_style_set_bg_opa(&styleGrid, LV_OPA_0);
        lv_style_set_border_opa(&styleGrid, LV_OPA_0);
        lv_obj_add_style(infoGrid, &styleGrid, LV_PART_MAIN);
        lv_obj_set_y(infoGrid,0);
        
        pdopLabel = lv_label_create(infoGrid);
        lv_obj_set_style_text_font(pdopLabel, fontSatInfo, 0);
        lv_label_set_text_fmt(pdopLabel, "PDOP:\n%s", "0.0");
        
        hdopLabel = lv_label_create(infoGrid);
        lv_obj_set_style_text_font(hdopLabel, fontSatInfo, 0);
        lv_label_set_text_fmt(hdopLabel, "HDOP:\n%s", "0.0");
        
        vdopLabel = lv_label_create(infoGrid);
        lv_obj_set_style_text_font(vdopLabel, fontSatInfo, 0);
        lv_label_set_text_fmt(vdopLabel, "VDOP:\n%s", "0.0");
        
        altLabel = lv_label_create(infoGrid);
        lv_obj_set_style_text_font(altLabel, fontSatInfo, 0);
        lv_label_set_text_fmt(altLabel, "ALT:\n%4dm.", 0); 
        
        satelliteBar1 = lv_chart_create(screen);
        lv_obj_set_size(satelliteBar1, ( TFT_WIDTH / 2 ) - 1 , 55);
        lv_chart_set_div_line_count(satelliteBar1, 6, 0);
        lv_chart_set_range(satelliteBar1, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
        satelliteBarSerie1 = lv_chart_add_series(satelliteBar1, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
        lv_chart_set_type(satelliteBar1, LV_CHART_TYPE_BAR);
        lv_chart_set_point_count(satelliteBar1,(MAX_SATELLLITES_IN_VIEW / 2));
        lv_obj_set_pos(satelliteBar1, 0, 155);

        satelliteBar2 = lv_chart_create(screen);
        lv_obj_set_size(satelliteBar2, ( TFT_WIDTH / 2 ) - 1 , 55);
        lv_chart_set_div_line_count(satelliteBar2, 6, 0);
        lv_chart_set_range(satelliteBar2, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
        satelliteBarSerie2 = lv_chart_add_series(satelliteBar2, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
        lv_chart_set_type(satelliteBar2, LV_CHART_TYPE_BAR);
        lv_chart_set_point_count(satelliteBar2, (MAX_SATELLLITES_IN_VIEW / 2));
        lv_obj_set_pos(satelliteBar2, TFT_WIDTH / 2, 155);
              
        #ifdef AT6558D_GPS
        lv_style_init(&styleRadio);
        lv_style_set_radius(&styleRadio, LV_RADIUS_CIRCLE);
        
        lv_style_init(&styleRadioChk);
        lv_style_set_bg_image_src(&styleRadioChk, NULL);
        
        lv_obj_t *gnssSel = lv_obj_create(screen);
        lv_obj_set_flex_flow(gnssSel, LV_FLEX_FLOW_ROW);
        lv_obj_set_size(gnssSel, 320, 50);
        lv_obj_set_pos(gnssSel, 0, 280);
        static lv_style_t styleSel;
        lv_style_init(&styleSel);
        lv_style_set_bg_opa(&styleSel, LV_OPA_0);
        lv_style_set_border_opa(&styleSel, LV_OPA_0);
        lv_obj_add_style(gnssSel, &styleSel, LV_PART_MAIN);
        
        lv_obj_t *gps = lv_checkbox_create(gnssSel);
        lv_checkbox_set_text_static(gps, "GPS     ");
        lv_obj_add_flag(gps, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_style(gps, &styleRadio, LV_PART_INDICATOR);
        lv_obj_add_style(gps, &styleRadioChk, LV_PART_INDICATOR | LV_STATE_CHECKED);
        
        lv_obj_t *glonass = lv_checkbox_create(gnssSel);
        lv_checkbox_set_text_static(glonass, "GLONASS  ");
        lv_obj_add_flag(glonass, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_style(glonass, &styleRadio, LV_PART_INDICATOR);
        lv_obj_add_style(glonass, &styleRadioChk, LV_PART_INDICATOR | LV_STATE_CHECKED);
        
        lv_obj_t *beidou = lv_checkbox_create(gnssSel);
        lv_checkbox_set_text_static(beidou, "BEIDOU");
        lv_obj_add_flag(beidou, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_style(beidou, &styleRadio, LV_PART_INDICATOR);
        lv_obj_add_style(beidou, &styleRadioChk, LV_PART_INDICATOR | LV_STATE_CHECKED);
        
        lv_obj_add_state(lv_obj_get_child(gnssSel, 0), LV_STATE_CHECKED);
        
        // GNSS Selection Event
        lv_obj_add_event_cb(gnssSel, activeGnssEvent, LV_EVENT_CLICKED, &activeGnss);
        #endif
    }
#endif