/**
 * @file lvgl.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL Screen implementation
 * @version 0.1
 * @date 2022-10-12
 */

#include "../lvgl/src/lvgl.h"
#include "gui/screens-lvgl/search_sat_scr.h"

static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 240;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];

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

    bool touched = tft.getTouch(&touchX, &touchY, 100);

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
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
}