/**
 * @file main_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Splash screen
 * @version 0.1
 * @date 2022-10-16
 */

#define MAX_TILES 3
int act_tile = 0;
int heading = 0;
int last_heading = 0;

#define UPDATE_MAINSCR_PERIOD 10
void update_main_screen(lv_timer_t *t);

static lv_obj_t *compass_heading;

/**
 * @brief Create a main screen
 *
 */
void create_main_scr()
{
    mainScreen = lv_obj_create(NULL);

    tiles = lv_tileview_create(mainScreen);
    lv_obj_t *compass = lv_tileview_add_tile(tiles, 0, 0, LV_DIR_RIGHT);
    lv_obj_t *map = lv_tileview_add_tile(tiles, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    lv_obj_t *sat_track = lv_tileview_add_tile(tiles, 2, 0, LV_DIR_LEFT);
    lv_obj_set_size(tiles, 240, 300);
    lv_obj_set_pos(tiles, 0, 20);

    compass_heading = lv_label_create(compass);
    lv_obj_set_size(compass_heading, 150, 48);
    lv_obj_set_align(compass_heading, LV_ALIGN_CENTER);
    lv_obj_set_y(compass_heading, 35);
    lv_obj_set_style_text_font(compass_heading, &lv_font_montserrat_48, 0);

    LV_IMG_DECLARE(arrow);
    lv_obj_t *img1 = lv_img_create(compass);
    lv_img_set_src(img1, &arrow);
    lv_obj_align(img1, LV_ALIGN_CENTER, 0, -20);

    lv_group_add_obj(group, tiles);
    lv_group_add_obj(group, mainScreen);

    lv_timer_t *t = lv_timer_create(update_main_screen, UPDATE_MAINSCR_PERIOD, NULL);
    lv_timer_ready(t);
}

/**
 * @brief Update Main Screen
 *
 */
void update_main_screen(lv_timer_t *t)
{
    lv_label_set_text_fmt(compass_heading, "%5d\xC2\xB0", heading);
}
