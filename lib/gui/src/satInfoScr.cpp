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

  //draw_rect_dsc.bg_color = lv_color_black();
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
            if ( strcmp(satTracker[dscId].talker_id,"GP") == 0 )
                fill_dsc->color = satTracker[dscId].active == true ? lv_color_hex(0x229954) : lv_color_hex(0x104828);
            if ( strcmp(satTracker[dscId].talker_id,"GL") == 0 )
                fill_dsc->color = satTracker[dscId].active == true ? lv_color_hex(0x2471a3) : lv_color_hex(0x11364d);
            if ( strcmp(satTracker[dscId].talker_id,"BD") == 0 )
                fill_dsc->color = satTracker[dscId].active == true ? lv_color_hex(0x7d3c98) : lv_color_hex(0x3b1c48);
            }
        }
    }
  }

  if (e == LV_EVENT_DRAW_POST_END) 
  {
    lv_layer_t * layer = lv_event_get_layer(event);
    char buf[16];
    
    for (uint16_t i = 0; i < gpsData.satInView; i++) 
    {
        lv_area_t chartObjCoords;
        lv_obj_get_coords(obj, &chartObjCoords);
        lv_point_t p;
        lv_chart_get_point_pos_by_id(obj, lv_chart_get_series_next(obj, NULL), i, &p);

        //Draw signal at top of bar
        if (satTracker[i].snr > 0)
        {
            lv_snprintf(buf, sizeof(buf), LV_SYMBOL_DUMMY"%d", satTracker[i].snr);
            drawTextOnLayer(buf, layer, &p, &chartObjCoords, lv_color_white(), fontSmall, 15);
        }

        //Draw Satellite ID
        lv_snprintf(buf, sizeof(buf), LV_SYMBOL_DUMMY"%d", satTracker[i].satNum);
        drawTextOnLayer(buf, layer, &p, &chartObjCoords, lv_color_white(), fontSmall, (chartObjCoords.y1 + p.y) - chartObjCoords.y2 + 10);
    }
  } 
}



 /**
 * @brief Satellite info screen
 *
 * @param screen 
 */
//  #ifndef TDECK_ESP32S3
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
        lv_obj_set_size(wrapper, TFT_WIDTH * 2, 150);
        lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);   

        lv_obj_t * gnssLabel = lv_label_create(barCont);
        lv_obj_set_style_text_font(gnssLabel, fontSatInfo, 0);
        lv_obj_set_pos(gnssLabel, 0, 127);
        lv_obj_set_width(gnssLabel,90);
        lv_obj_set_style_bg_color(gnssLabel, lv_color_hex(0x104828), 0);
        lv_obj_set_style_bg_opa(gnssLabel, LV_OPA_100, 0);
        lv_obj_set_style_border_color(gnssLabel, lv_color_hex(0x229954), 0);
        lv_obj_set_style_border_width(gnssLabel, 1, 0);
        lv_obj_set_style_border_opa(gnssLabel, LV_OPA_100, 0);
        lv_label_set_text(gnssLabel, "GPS");
        lv_obj_set_style_text_align(gnssLabel, LV_TEXT_ALIGN_CENTER, 0);

        gnssLabel = lv_label_create(barCont);
        lv_obj_set_style_text_font(gnssLabel, fontSatInfo, 0);
        lv_obj_set_pos(gnssLabel, 95, 127);
        lv_obj_set_width(gnssLabel,90);
        lv_obj_set_style_bg_color(gnssLabel, lv_color_hex(0x11364d), 0);
        lv_obj_set_style_bg_opa(gnssLabel, LV_OPA_100, 0);
        lv_obj_set_style_border_color(gnssLabel, lv_color_hex(0x2471a3), 0);
        lv_obj_set_style_border_width(gnssLabel, 1, 0);
        lv_obj_set_style_border_opa(gnssLabel, LV_OPA_100, 0);
        lv_label_set_text(gnssLabel, "GLONASS");
        lv_obj_set_style_text_align(gnssLabel, LV_TEXT_ALIGN_CENTER, 0);

        gnssLabel = lv_label_create(barCont);
        lv_obj_set_style_text_font(gnssLabel, fontSatInfo, 0);
        lv_obj_set_pos(gnssLabel, 190, 127);
        lv_obj_set_width(gnssLabel,90);
        lv_obj_set_style_bg_color(gnssLabel, lv_color_hex(0x3b1c48), 0);
        lv_obj_set_style_bg_opa(gnssLabel, LV_OPA_100, 0);
        lv_obj_set_style_border_color(gnssLabel, lv_color_hex(0x7d3c98), 0);
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
// #endif

// #ifdef TDECK_ESP32S3
//     void satelliteScr(_lv_obj_t *screen)
//     {
//         lv_obj_t *infoGrid = lv_obj_create(screen);
//         lv_obj_set_size(infoGrid, 90, 155);
//         lv_obj_set_flex_align(infoGrid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
//         lv_obj_set_style_pad_row(infoGrid, 5 * scale, 0);
//         lv_obj_clear_flag(infoGrid, LV_OBJ_FLAG_SCROLLABLE);
//         lv_obj_set_flex_flow(infoGrid, LV_FLEX_FLOW_COLUMN);
//         static lv_style_t styleGrid;
//         lv_style_init(&styleGrid);
//         lv_style_set_bg_opa(&styleGrid, LV_OPA_0);
//         lv_style_set_border_opa(&styleGrid, LV_OPA_0);
//         lv_obj_add_style(infoGrid, &styleGrid, LV_PART_MAIN);
//         lv_obj_set_y(infoGrid,0);
        
//         pdopLabel = lv_label_create(infoGrid);
//         lv_obj_set_style_text_font(pdopLabel, fontSatInfo, 0);
//         lv_label_set_text_fmt(pdopLabel, "PDOP:\n%s", "0.0");
        
//         hdopLabel = lv_label_create(infoGrid);
//         lv_obj_set_style_text_font(hdopLabel, fontSatInfo, 0);
//         lv_label_set_text_fmt(hdopLabel, "HDOP:\n%s", "0.0");
        
//         vdopLabel = lv_label_create(infoGrid);
//         lv_obj_set_style_text_font(vdopLabel, fontSatInfo, 0);
//         lv_label_set_text_fmt(vdopLabel, "VDOP:\n%s", "0.0");
        
//         altLabel = lv_label_create(infoGrid);
//         lv_obj_set_style_text_font(altLabel, fontSatInfo, 0);
//         lv_label_set_text_fmt(altLabel, "ALT:\n%4dm.", 0); 
        
//         satelliteBar1 = lv_chart_create(screen);
//         lv_obj_set_size(satelliteBar1, ( TFT_WIDTH / 2 ) - 1 , 55);
//         lv_chart_set_div_line_count(satelliteBar1, 6, 0);
//         lv_chart_set_range(satelliteBar1, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
//         satelliteBarSerie1 = lv_chart_add_series(satelliteBar1, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
//         lv_chart_set_type(satelliteBar1, LV_CHART_TYPE_BAR);
//         lv_chart_set_point_count(satelliteBar1,(MAX_SATELLLITES_IN_VIEW / 2));
//         lv_obj_set_pos(satelliteBar1, 0, 155);

//         satelliteBar2 = lv_chart_create(screen);
//         lv_obj_set_size(satelliteBar2, ( TFT_WIDTH / 2 ) - 1 , 55);
//         lv_chart_set_div_line_count(satelliteBar2, 6, 0);
//         lv_chart_set_range(satelliteBar2, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
//         satelliteBarSerie2 = lv_chart_add_series(satelliteBar2, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
//         lv_chart_set_type(satelliteBar2, LV_CHART_TYPE_BAR);
//         lv_chart_set_point_count(satelliteBar2, (MAX_SATELLLITES_IN_VIEW / 2));
//         lv_obj_set_pos(satelliteBar2, TFT_WIDTH / 2, 155);
              
//     }
// #endif