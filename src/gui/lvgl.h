/**
 * @file lvgl.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL Screen implementation
 * @version 0.1
 * @date 2022-10-12
 */
#include <lvgl.h>

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
static lv_obj_t *mainScreen;
static lv_obj_t *tiles;

#include "gui/img/arrow.c"
#include "gui/img/position.c"
#include "gui/img/bruj.c"
#include "gui/img/navigation.c"
#include "gui/screens-lvgl/notify_bar.h"
#include "gui/screens-lvgl/search_sat_scr.h"
#include "gui/screens-lvgl/main_scr.h"
#include "gui/screens-lvgl/splash_scr.h"

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
    if (tft.getStartCount() == 0)
    {
        tft.startWrite();
//        tft.endWrite();
    }
    tft.pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (uint16_t *)&color_p->full);
    lv_disp_flush_ready(disp); 
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
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);
    
    //  Create Screens //
    create_search_sat_scr();
    create_main_scr();
}
