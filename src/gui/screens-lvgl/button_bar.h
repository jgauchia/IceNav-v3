/**
 * @file button_bar.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Button Bar
 * @version 0.1
 * @date 2023-03-27
 */

/**
 * @brief Button definition
 *
 */

/**
 * @brief Create  button bar
 *
 */
void create_button_bar()
{

    lv_obj_t *imgbtn1 = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(imgbtn1, LV_IMGBTN_STATE_RELEASED, "F:/config.bin" ,NULL, NULL);
    lv_obj_set_size(imgbtn1, 64,64);
    lv_obj_set_pos(imgbtn1, 0, TFT_HEIGHT - 75);
}
