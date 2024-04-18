/**
 * @file lvgl.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL Screen implementation
 * @version 0.1.8
 * @date 2024-04
 */

#include <lvgl.h>

/**
 * @brief Default display driver definition
 *
 */

static lv_display_t *display;
#define DRAW_BUF_SIZE (TFT_WIDTH * TFT_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
// static uint32_t *drawBuf = (uint32_t *)ps_malloc(TFT_WIDTH * TFT_HEIGHT / 10 * (LV_COLOR_DEPTH / 8));
uint32_t drawBuf[DRAW_BUF_SIZE / 4];


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
bool isMainScreen = false;

/**
 * @brief Main Timer
 *
 */
static lv_timer_t *mainTimer;

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
static lv_obj_t *mapSettingsScreen;
static lv_obj_t *deviceSettingsScreen;

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
void displayFlush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
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
void touchRead(lv_indev_t *indev_driver, lv_indev_data_t *data)
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
void initLVGL()
{
    lv_init();

    lv_port_spiffsFsInit();
    // lv_port_sdFsInit();

    display = lv_display_create(TFT_WIDTH, TFT_HEIGHT);
    lv_display_set_flush_cb(display, displayFlush);
    lv_display_set_buffers(display, drawBuf, NULL, sizeof(drawBuf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev_drv = lv_indev_create();
    lv_indev_set_type(indev_drv, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_drv, touchRead);

    //  Create Main Timer
    mainTimer = lv_timer_create(updateMainScreen, UPDATE_MAINSCR_PERIOD, NULL);
    lv_timer_ready(mainTimer);

    //  Create Screens
    createSearchSatScr();
    createMainScr();
    createNotifyBar();
    createSettingsScr();
    createMapSettingsScr();
    createDeviceSettingsScr();
    createButtonBarScr();
}

/**
 * @brief Load GPS Main Screen
 *
 */
void loadMainScreen()
{
    isMainScreen = true;
    lv_screen_load(mainScreen);
}
