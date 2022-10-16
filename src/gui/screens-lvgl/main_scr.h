/**
 * @file main_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Splash screen
 * @version 0.1
 * @date 2022-10-16
 */

#define MAX_TILES 3
int act_tile = 0;

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

    lv_group_add_obj(group, tiles);
    lv_group_add_obj(group, mainScreen);
}