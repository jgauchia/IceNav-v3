/**
 * @file lvgl.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL Screen implementation
 * @version 0.1
 * @date 2022-10-12
 */

//#include "../lvgl/src/lvgl.h"
#include <lvgl.h>

static const uint16_t screenWidth = TFT_WIDTH;
static const uint16_t screenHeight = TFT_HEIGHT;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];
static lv_obj_t *currentScreen;
static lv_group_t *group;
static lv_indev_t *my_indev;
static lv_obj_t *mainScreen;
static lv_obj_t *tiles;

#include "gui/img/arrow.c"
#include "gui/img/position.c"
#include "gui/img/bruj.c"
#include "gui/screens-lvgl/notify_bar.h"
#include "gui/screens-lvgl/search_sat_scr.h"
#include "gui/screens-lvgl/splash_scr.h"
#include "gui/screens-lvgl/main_scr.h"

#define LVGL_TICK_PERIOD 10
Ticker tick;
SemaphoreHandle_t xSemaphore = NULL;
static void lv_tick_handler(void)
{
    lv_tick_inc(LVGL_TICK_PERIOD);
}

/**
 * @brief LVGL display update
 *
 */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
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
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX, touchY;

    bool touched;// = tft.getTouch(&touchX, &touchY, 100);

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
void my_keypad_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    currentScreen = lv_scr_act();
    key_pressed = Read_Keys();
    switch (key_pressed)
    {
    case LEFT:
        if (currentScreen == mainScreen)
        {
            act_tile--;
            if (act_tile < 0)
                act_tile = 0;
            lv_obj_set_tile_id(tiles, act_tile, 0, LV_ANIM_ON);
        }
        // data->key = LV_KEY_NEXT;
        break;
    case RIGHT:
        if (currentScreen == mainScreen)
        {
            act_tile++;
            if (act_tile > MAX_TILES - 1)
                act_tile = MAX_TILES;
            lv_obj_set_tile_id(tiles, act_tile, 0, LV_ANIM_ON);
        }
        break;
    case UP:
        data->key = LV_KEY_UP;
        break;
    case DOWN:
        data->key = LV_KEY_DOWN;
        break;
    case PUSH:
        break;
    case LUP:
        if (currentScreen == mainScreen && act_tile == 1)
        {
            zoom++;
            if (zoom > MAX_ZOOM)
                zoom = MAX_ZOOM;
        }
        break;
    case LDOWN:
        if (currentScreen == mainScreen && act_tile == 1)
        {
            zoom--;
            if (zoom < MIN_ZOOM)
                zoom = MIN_ZOOM;
        }
        break;
    case LBUT:
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

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    //  Init Screen  //
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    //  Init input device //
    static lv_indev_drv_t indev_drv;

    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = my_keypad_read;
    my_indev = lv_indev_drv_register(&indev_drv);

    create_search_sat_scr();
    create_splash_scr();
    create_main_scr();

    group = lv_group_create();
    lv_group_set_default(group);
    lv_indev_set_group(my_indev, group);

    tick.attach_ms(LVGL_TICK_PERIOD, lv_tick_handler);
    xSemaphore = xSemaphoreCreateMutex();
}
