/**
 * @file main_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Splash screen
 * @version 0.1
 * @date 2022-10-16
 */

static lv_obj_t *mainScreen;
#define MAX_TILES 3
int act_tile = 0;

/**
 * @brief Create a main screen
 *
 */
void create_main_scr()
{
    mainScreen = lv_tileview_create(NULL);
    lv_obj_t *compass = lv_tileview_add_tile(mainScreen, 0, 0, LV_DIR_RIGHT);
    lv_obj_t *map = lv_tileview_add_tile(mainScreen, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    lv_obj_t *sat_track = lv_tileview_add_tile(mainScreen, 2, 0, LV_DIR_LEFT);

    lv_obj_t *label_batt = lv_label_create(compass);
    lv_label_set_text(label_batt, LV_SYMBOL_BATTERY_EMPTY);
    lv_obj_set_pos(label_batt, 215, 0);
    lv_obj_t *label_batt2 = lv_label_create(map);
    lv_label_set_text(label_batt2, LV_SYMBOL_BATTERY_EMPTY);
    lv_obj_set_pos(label_batt2, 215, 0);

    lv_group_add_obj(group, mainScreen);
}