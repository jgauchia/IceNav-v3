/**
 * @file main_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Main screen events
 * @version 0.1
 * @date 2022-12-06
 */

/**
 * @brief Get the active tile
 *
 * @param event
 */
static void get_act_tile(lv_event_t *event)
{
    lv_obj_t *acttile = lv_tileview_get_tile_act(tiles);
    lv_coord_t tile_x = lv_obj_get_x(acttile) / TFT_WIDTH;
    act_tile = tile_x;
    if (act_tile == MAP)
        is_map_draw = false;
}