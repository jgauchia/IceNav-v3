/**
 * @file lvgl.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL Screen implementation
 * @version 0.1.6
 * @date 2023-06-14
 */

#include <lvgl.h>

/**
 * @brief Default display driver definition
 *
 */
static const uint16_t screenWidth = TFT_WIDTH;
static const uint16_t screenHeight = TFT_HEIGHT;
static lv_disp_draw_buf_t draw_buf;

static lv_obj_t *currentScreen;
static lv_group_t *group;
static lv_obj_t *mainScreen;
static lv_obj_t *tiles;
static lv_disp_drv_t def_drv;

/**
 * @brief Main Timer
 *
 */
static lv_timer_t *timer_main;

/**
 * @brief  Main Screen update time
 *
 */
#define UPDATE_MAINSCR_PERIOD 30

#include "gui/img/bruj.c"
#include "gui/img/navigation.c"
#include "gui/img/compass.c"
#include "gui/img/zoom.c"
#include "gui/img/speed.c"
#include "gui/screens-lvgl/notify_bar.h"
#include "gui/screens-lvgl/button_bar.h"
#include "gui/screens-lvgl/search_sat_scr.h"
#include "gui/screens-lvgl/main_scr.h"
#include "gui/screens-lvgl/splash_scr.h"

/**
 * @brief LVGL display update
 *
 */
void disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    // if (!lv_disp_is_invalidation_enabled(disp_ctrl))
    {
        tft.startWrite();
        // Map Tile, refresh partial screen to avoid flickering when scroll tile view
        // if (act_tile == MAP && is_scrolled)
        // {
        //     if ((area->y2 - area->y1 + 1) < 20)
        //         tft.pushImage(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (uint16_t *)&color_p->full);
        // }
        // else
        {
            tft.pushImage(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (uint16_t *)&color_p->full);
        }
        tft.endWrite();
        lv_disp_flush_ready(disp);
    }
    // uint32_t w = (area->x2 - area->x1 + 1);
    // uint32_t h = (area->y2 - area->y1 + 1);

    // tft.startWrite();
    // tft.setAddrWindow(area->x1, area->y1, w, h);
    // tft.pushPixelsDMA((uint16_t *)&color_p->full, w * h, false);
    // tft.waitDMA();
    // tft.endWrite();
    // lv_disp_flush_ready(disp);
}

/**
 * @brief LVGL touch read
 *
 */
void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX, touchY;
    bool touched = tft.getTouch(&touchX, &touchY);
    if (!touched)
        data->state = LV_INDEV_STATE_RELEASED;
    else
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = touchX;
        data->point.y = touchY;
        // log_v("X: %04i, Y: %04i",touchX, touchY);
    }
}

/**
 * @brief Init LVGL
 *
 */
void init_LVGL()
{
    lv_init();

    lv_port_spiffs_fs_init();
    // lv_port_sd_fs_init();

    static lv_color_t *buf1 = (lv_color_t *)ps_malloc((TFT_WIDTH * TFT_HEIGHT) * 2);
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, (TFT_WIDTH * TFT_HEIGHT) * 2);

    lv_disp_drv_init(&def_drv);
    def_drv.hor_res = screenWidth;
    def_drv.ver_res = screenHeight;
    def_drv.flush_cb = disp_flush;
    def_drv.draw_buf = &draw_buf;
    def_drv.full_refresh = 0;
    lv_disp_drv_register(&def_drv);

    //  Init input device //
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    //  Create Main Timer
    timer_main = lv_timer_create(update_main_screen, UPDATE_MAINSCR_PERIOD, NULL);
    lv_timer_ready(timer_main);

    //  Create Screens //
    create_search_sat_scr();
    create_main_scr();
}

/**
 * @brief Load GPS Main Screen
 * 
 */
void load_main_screen()
{
    lv_scr_load(mainScreen);
    create_button_bar_scr();
    create_notify_bar();
}