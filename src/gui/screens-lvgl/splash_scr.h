/**
 * @file splash_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Splash screen
 * @version 0.1
 * @date 2022-10-14
 */

/**
 * @brief Create a splash screen
 *
 */
void create_splash_scr()
{
    splash = lv_img_create(lv_scr_act());
    lv_img_set_src(splash, "S:/INIT.BMP");
    // #endif
    //lv_obj_set_pos(splash, 0, 0);
}