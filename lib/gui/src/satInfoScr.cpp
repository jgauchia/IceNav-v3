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
        lv_label_set_text_fmt(pdopLabel, "PDOP:\n%.1f", 0);
        
        hdopLabel = lv_label_create(infoGrid);
        lv_obj_set_style_text_font(hdopLabel, fontSatInfo, 0);
        lv_label_set_text_fmt(hdopLabel, "HDOP:\n%.1f", 0);
        
        vdopLabel = lv_label_create(infoGrid);
        lv_obj_set_style_text_font(vdopLabel, fontSatInfo, 0);
        lv_label_set_text_fmt(vdopLabel, "VDOP:\n%.1f", 0);
        
        altLabel = lv_label_create(infoGrid);
        lv_obj_set_style_text_font(altLabel, fontSatInfo, 0);
        lv_label_set_text_fmt(altLabel, "ALT:\n%4dm.", 0); 
        
        satelliteBar1 = lv_chart_create(screen);
        lv_obj_set_size(satelliteBar1, TFT_WIDTH, 55 * scale);
        lv_chart_set_div_line_count(satelliteBar1, 6, 0);
        lv_chart_set_range(satelliteBar1, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
        satelliteBarSerie1 = lv_chart_add_series(satelliteBar1, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
        lv_chart_set_type(satelliteBar1, LV_CHART_TYPE_BAR);
        lv_chart_set_point_count(satelliteBar1, (MAX_SATELLLITES_IN_VIEW / 2));
        lv_obj_set_pos(satelliteBar1, 0, 175 * scale);
        
        satelliteBar2 = lv_chart_create(screen);
        lv_obj_set_size(satelliteBar2, TFT_WIDTH, 55 * scale);
        lv_chart_set_div_line_count(satelliteBar2, 6, 0);
        lv_chart_set_range(satelliteBar2, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
        satelliteBarSerie2 = lv_chart_add_series(satelliteBar2, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
        lv_chart_set_type(satelliteBar2, LV_CHART_TYPE_BAR);
        lv_chart_set_point_count(satelliteBar2, (MAX_SATELLLITES_IN_VIEW / 2));
        lv_obj_set_pos(satelliteBar2, 0, 260 * scale);
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
              
    }
#endif