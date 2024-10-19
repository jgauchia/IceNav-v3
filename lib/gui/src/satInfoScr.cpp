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

void drawTextOnLayer(const char * text, lv_layer_t * layer, lv_point_t * p, lv_area_t * coords, lv_color_t color, const void * font, int16_t offset)
{
  lv_draw_rect_dsc_t drawRectDsc;
  lv_draw_rect_dsc_init(&drawRectDsc);

  //drawRectDsc.bg_color = lv_color_black();
  drawRectDsc.bg_opa = LV_OPA_TRANSP;
  drawRectDsc.radius = 0;
  drawRectDsc.bg_image_symbol_font = font;
  drawRectDsc.bg_image_src = text;
  drawRectDsc.bg_image_recolor = color;

  lv_area_t a;
  a.x1 = coords->x1 + p->x - 10;
  a.x2 = coords->x1 + p->x + 10;
  a.y1 = coords->y1 + p->y + 10 - offset;
  a.y2 = a.y1 - 20;

  lv_draw_rect(layer, &drawRectDsc, &a);
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

  if (e == LV_EVENT_DRAW_TASK_ADDED && drawSNRBar) 
  {
    lv_draw_task_t * drawTask = lv_event_get_draw_task(event);
    lv_draw_dsc_base_t * base_dsc = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(drawTask);

    if(base_dsc->part == LV_PART_ITEMS)
    {
      uint16_t dscId = base_dsc->id1;
      if (lv_draw_task_get_type(drawTask) == LV_DRAW_TASK_TYPE_FILL) 
      {
        lv_draw_fill_dsc_t * fill_dsc = lv_draw_task_get_fill_dsc(drawTask);
        if(fill_dsc) 
        {
          switch (satTracker[dscId].type)
          {
             case 1:
              fill_dsc->color = lv_color_hex(0x196f3d);
              break;
             case 0:
              fill_dsc->color = lv_color_hex(0x1a5276);
              break;
             case 2:
              fill_dsc->color = lv_color_hex(0x5b2c6f);
              break;
          }
        }
      }
      if (lv_draw_task_get_type(drawTask) == LV_DRAW_TASK_TYPE_BORDER) 
      {
        lv_draw_border_dsc_t * border_dsc = lv_draw_task_get_border_dsc(drawTask);
        if (border_dsc) 
        {
          border_dsc->width = satTracker[dscId].active == true ? 1 : 0;
          border_dsc->color = satTracker[dscId].active == true ? lv_color_white() : lv_color_black();
        }
      }
    }
  }

  if (e == LV_EVENT_DRAW_POST_END && drawSNRBar) 
  {
    lv_layer_t * layer = lv_event_get_layer(event);
    char buf[16];

    for (int i = 0; i < totalSatView; i++)
    {
      lv_area_t chartObjCoords;
      lv_obj_get_coords(obj, &chartObjCoords);
      lv_point_t p;
      lv_chart_get_point_pos_by_id(obj, lv_chart_get_series_next(obj, NULL), i, &p);

      // Satellite SNR
      if (satTracker[i].snr != 0 )
      {
        lv_snprintf(buf, sizeof(buf), LV_SYMBOL_DUMMY"%d", satTracker[i].snr);
        drawTextOnLayer(buf, layer, &p, &chartObjCoords, lv_color_white(), fontSmall, 15);
      }

      // Satellite Id
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
        
        lv_obj_t * barCont = lv_obj_create(screen);
        lv_obj_set_size(barCont, TFT_WIDTH, 180 * scale);
        lv_obj_set_pos(barCont, 0, 200 * scale);
        lv_obj_t * wrapper = lv_obj_create(barCont);
        lv_obj_remove_style_all(wrapper);
        lv_obj_set_size(wrapper, TFT_WIDTH * 2, 150);
        lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);

        satelliteBar = lv_chart_create(wrapper);
        lv_obj_set_size(satelliteBar, TFT_WIDTH * 2, 120 * scale);
        lv_chart_set_div_line_count(satelliteBar, 3, 0);
        lv_chart_set_range(satelliteBar, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
        satelliteBarSerie = lv_chart_add_series(satelliteBar, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
        lv_chart_set_type(satelliteBar, LV_CHART_TYPE_BAR);
        //lv_obj_set_style_pad_all(satelliteBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_column(satelliteBar, 2, 0);
        lv_chart_set_point_count(satelliteBar, MAX_SATELLLITES_IN_VIEW );
        lv_obj_add_event_cb(satelliteBar, satelliteBarDrawEvent, LV_EVENT_DRAW_TASK_ADDED, NULL);
        lv_obj_add_event_cb(satelliteBar, satelliteBarDrawEvent, LV_EVENT_DRAW_POST_END, NULL);
        lv_obj_add_flag(satelliteBar, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    }
// #endif

// #ifdef TDECK_ESP32S3
// #endif