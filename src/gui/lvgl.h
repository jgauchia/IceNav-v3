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

static lv_display_t *display;
#define DRAW_BUF_SIZE (TFT_WIDTH * TFT_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
//static uint32_t *draw_buf = (uint32_t *)ps_malloc(TFT_WIDTH * TFT_HEIGHT / 10 * (LV_COLOR_DEPTH / 8));
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// static lv_color_t *buf1 = (lv_color_t *)ps_malloc((TFT_WIDTH * TFT_HEIGHT) * 2);
// static lv_disp_draw_buf_t draw_buf;
// static lv_disp_drv_t def_drv;

/**
 * @brief Screens definitions
 * 
 */
static lv_obj_t *currentScreen;
static lv_group_t *group;
static lv_obj_t *mainScreen;
static lv_obj_t *tiles;

/**
 * @brief Flag to indicate main screen is selected
 *
 */
bool is_main_screen = false;

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

/**
 * @brief Extra screen definitions
 *
 */
static lv_obj_t *settingsScreen;
static lv_obj_t *mapsettingsScreen;
static lv_obj_t *devicesettingsScreen;

#include "gui/lvgl_funcs.h"
#include "gui/images/bruj.c"
#include "gui/images/navigation.c"
#include "gui/images/compass.c"
#include "gui/images/zoom.c"
#include "gui/images/speed.c"

#include "gui/screens/Notify_Bar/notify_bar.h"
#include "gui/screens/Map_Settings/map_settings.h"
#include "gui/screens/Device_Settings/device_settings.h"
#include "gui/screens/Settings_Menu/settings_scr.h"
#include "gui/screens/Main/main_scr.h"
#include "gui/screens/Button_Bar/button_bar.h"
#include "gui/screens/Search_Satellite/search_sat_scr.h"
#include "gui/screens/Splash/splash_scr.h"

/**
 * @brief LVGL display update
 *
 */
void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    // tft.startWrite();
    // tft.pushImage(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (uint16_t)px_map);
    // tft.endWrite();

    // uint32_t w = (area->x2 - area->x1 + 1);
    // uint32_t h = (area->y2 - area->y1 + 1);
    //  uint16_t * buf16 = (uint16_t*)px_map;

    // tft.startWrite();
    // tft.setAddrWindow(area->x1, area->y1, w, h);
    // tft.pushPixels(buf16, w * h, true);
    // tft.endWrite();

    // lv_disp_flush_ready(disp);

    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushPixels((uint16_t *)px_map, w * h, true);
    tft.endWrite();
    lv_display_flush_ready(disp);

    // if (tft.getStartCount() == 0)
    //     tft.startWrite();

    // tft.pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (lgfx::swap565_t *)&color_p->full);

    // if (tft.getStartCount() > 0)
    //     tft.endWrite();

    // lv_disp_flush_ready(disp);
}

/**
 * @brief LVGL touch read
 *
 */
void touchpad_read(lv_indev_t *indev_driver, lv_indev_data_t *data)
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

    display = lv_display_create(TFT_WIDTH, TFT_HEIGHT);
    lv_display_set_flush_cb(display, disp_flush);
    lv_display_set_buffers(display, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // static lv_color_t *buf1 = (lv_color_t *)ps_malloc((TFT_WIDTH * TFT_HEIGHT) * 2);
    // lv_disp_draw_buf_init(&draw_buf, buf1, NULL, (TFT_WIDTH * TFT_HEIGHT) * 2);
    // lv_disp_drv_init(&def_drv);
    // def_drv.hor_res = screenWidth;
    // def_drv.ver_res = screenHeight;
    // def_drv.flush_cb = disp_flush;
    // def_drv.draw_buf = &draw_buf;
    // def_drv.full_refresh = 0;
    // lv_disp_drv_register(&def_drv);

    //  Init input device //
    // static lv_indev_drv_t indev_drv;
    // lv_indev_drv_init(&indev_drv);
    //     indev_drv.type = LV_INDEV_TYPE_POINTER;
    // indev_drv.read_cb = touchpad_read;
    // lv_indev_drv_register(&indev_drv);
    lv_indev_t *indev_drv = lv_indev_create();
    lv_indev_set_type(indev_drv, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_drv, touchpad_read);

    //  Create Main Timer
    // timer_main = lv_timer_create(update_main_screen, UPDATE_MAINSCR_PERIOD, NULL);
    // lv_timer_ready(timer_main);

    //  Create Screens
    create_search_sat_scr();
    create_main_scr();
    create_notify_bar();
}   // create_settings_scr();
    // create_map_settings_scr();
    // create_device_settings_scr();

    // create_button_bar_scr();
 
/**
 * @brief Load GPS Main Screen
 *
 */
void load_main_screen()
{
    is_main_screen = true;
    lv_screen_load(mainScreen);
}
