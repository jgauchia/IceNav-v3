/**
 * @file button_bar.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Button Bar
 * @version 0.1
 * @date 2023-03-27
 */

/**
 * @brief Create  button bar
 *
 */
void create_button_bar()
{
    static lv_style_t style_pr;
    lv_style_init(&style_pr);
    lv_style_set_img_recolor_opa(&style_pr, LV_OPA_40);

    lv_obj_t *navbtn = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(navbtn, LV_IMGBTN_STATE_RELEASED, "F:/nav.bin", "F:/nav.bin", "F:/nav.bin");
    lv_obj_add_style(navbtn, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_size(navbtn, 48, 48);
    lv_obj_set_pos(navbtn, 16, TFT_HEIGHT - 64);

    lv_obj_t *wptbtn = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(wptbtn, LV_IMGBTN_STATE_RELEASED, "F:/wpt.bin", "F:/wpt.bin", "F:/wpt.bin");
    lv_obj_add_style(wptbtn, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_size(wptbtn, 48, 48);
    lv_obj_set_pos(wptbtn, 80, TFT_HEIGHT - 64);

    lv_obj_t *cfgbtn = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(cfgbtn, LV_IMGBTN_STATE_RELEASED, "F:/config.bin", "F:/config.bin", "F:/config.bin");
    lv_obj_add_style(cfgbtn, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_size(cfgbtn, 48, 48);
    lv_obj_set_pos(cfgbtn, TFT_WIDTH - 64, TFT_HEIGHT - 64);
}
