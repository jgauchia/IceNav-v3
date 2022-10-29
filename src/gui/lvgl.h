/**
 * @file lvgl.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL Screen implementation
 * @version 0.1
 * @date 2022-10-12
 */
#include <lvgl.h>

/**
 * @brief Active Tile in TileView control
 *
 */
#define MAX_TILES 3
int act_tile = 0;
enum tilename
{
    COMPASS,
    MAP,
    SATTRACK,
};

/**
 * @brief Default display driver definition
 *
 */
static const uint16_t screenWidth = TFT_WIDTH;
static const uint16_t screenHeight = TFT_HEIGHT;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];

static lv_obj_t *currentScreen;
static lv_group_t *group;
static lv_indev_t *my_indev;
static lv_obj_t *mainScreen;
static lv_obj_t *tiles;

/**
 * @brief Flag to indicate when maps needs to be draw
 *
 */
bool is_map_draw = false;

#include "gui/img/arrow.c"
#include "gui/img/position.c"
#include "gui/img/bruj.c"
#include "gui/img/zoom.c"
#include "gui/img/navigation.c"
#include "gui/screens-lvgl/notify_bar.h"
#include "gui/screens-lvgl/search_sat_scr.h"
#include "gui/screens-lvgl/main_scr.h"
#include "gui/screens-lvgl/splash_scr.h"

/**
 * @brief Task timer for LVGL screen update
 *
 */
#define LVGL_TICK_PERIOD 2
Ticker tick;
SemaphoreHandle_t xSemaphore = NULL;
static void lv_tick_handler(void)
{
    lv_tick_inc(LVGL_TICK_PERIOD);
}

/**
 * @brief Runtime change LVGL screen resolution
 *
 * @param width
 * @param height
 */
void lvgl_set_resolution(int width, int height)
{
    lv_disp_t *def_disp;
    def_disp = lv_disp_get_default();
    lv_disp_drv_t *lv_disp_drv;
    lv_disp_drv = def_disp->driver;
    lv_disp_drv->hor_res = width;
    lv_disp_drv->ver_res = height;
    lv_disp_drv_update(def_disp, lv_disp_drv);
}

/**
 * @brief LVGL display update
 *
 */
void disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

/**
 * @brief LVGL touch read
 *
 */
void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX, touchY;

    bool touched; // = tft.getTouch(&touchX, &touchY, 100);

    if (!touched)
        data->state = LV_INDEV_STATE_REL;
    else
    {
        data->state = LV_INDEV_STATE_PR;

        data->point.x = touchX;
        data->point.y = touchY;
    }
}

/**
 * @brief LVGL Keypad read
 *
 */
void keypad_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    currentScreen = lv_scr_act();
    key_pressed = Read_Keys();

    if (key_pressed > 0)
        data->state = LV_INDEV_STATE_PRESSED;
    else
        data->state = LV_INDEV_STATE_RELEASED;

    switch (key_pressed)
    {
    case LEFT:
        if (currentScreen == mainScreen)
        {
            act_tile--;
            if (act_tile < 0)
                act_tile = 0;
            if (act_tile == MAP)
            {
                lvgl_set_resolution(TFT_WIDTH, 63);
                tft.fillScreen(TFT_BLACK);
            }
            else
            {
                lvgl_set_resolution(TFT_WIDTH, TFT_HEIGHT);
                is_map_draw = false;
            }
            lv_obj_set_tile_id(tiles, act_tile, 0, LV_ANIM_OFF);
        }
        // data->key = LV_KEY_PREV;
        break;
    case RIGHT:
        if (currentScreen == mainScreen)
        {
            act_tile++;
            if (act_tile > MAX_TILES - 1)
                act_tile = MAX_TILES;
            if (act_tile == MAP)
            {
                lvgl_set_resolution(TFT_WIDTH, 63);
                tft.fillScreen(TFT_BLACK);
            }
            else
            {
                lvgl_set_resolution(TFT_WIDTH, TFT_HEIGHT);
                is_map_draw = false;
            }
            lv_obj_set_tile_id(tiles, act_tile, 0, LV_ANIM_OFF);
        }
        // data->key = LV_KEY_NEXT;
        break;
    case UP:
        // data->key = LV_KEY_UP;
        break;
    case DOWN:
        // data->key = LV_KEY_DOWN;
        break;
    case PUSH:
        break;
    case LUP:
        if (currentScreen == mainScreen && act_tile == MAP)
        {
            data->key = LV_KEY_UP;
        }
        break;
    case LDOWN:
        if (currentScreen == mainScreen && act_tile == MAP)
        {
            data->key = LV_KEY_DOWN;
        }
        break;
    case LBUT:
        data->key = 0;
        break;
    default:
        data->key = 0;
        break;
    }
}

/**
 * @brief Init LVGL
 *
 */
void init_LVGL()
{
    lv_init();

    //  Init Default Screen driver //
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);
    static lv_disp_drv_t def_drv;
    lv_disp_drv_init(&def_drv);
    def_drv.hor_res = screenWidth;
    def_drv.ver_res = screenHeight;
    def_drv.flush_cb = disp_flush;
    def_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&def_drv);

    //  Init input device //
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keypad_read;
    my_indev = lv_indev_drv_register(&indev_drv);

    //  Create Screens //
    create_search_sat_scr();
    create_main_scr();

    group = lv_group_create();
    lv_group_add_obj(group, zoombox);
    lv_indev_set_group(my_indev, group);
    lv_group_set_default(group);

    tick.attach_ms(LVGL_TICK_PERIOD, lv_tick_handler);
    xSemaphore = xSemaphoreCreateMutex();
}
