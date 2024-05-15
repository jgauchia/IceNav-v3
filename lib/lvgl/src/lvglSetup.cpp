/**
 * @file lvglSetup.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL Screen implementation
 * @version 0.1.8
 * @date 2024-05
 */

#include "lvglSetup.hpp"

ViewPort viewPort; // Vector map viewport
MemCache memCache; // Vector map Memory Cache

lv_obj_t *searchSatScreen; // Search Satellite Screen
lv_style_t styleThemeBkg;  // New Main Background Style
lv_style_t styleObjectBkg; // New Objects Background Color
lv_style_t styleObjectSel; // New Objects Selected Color

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
    }
}

/**
 * @brief Apply Custom Dark theme
 *
 * @param th
 * @param obj
 */
void applyModifyTheme(lv_theme_t *th, lv_obj_t *obj)
{
    LV_UNUSED(th);
    if (!lv_obj_check_type(obj, &lv_led_class))
    {
        if (!lv_obj_check_type(obj, &lv_button_class))
            lv_obj_add_style(obj, &styleThemeBkg, 0);
        if (lv_obj_check_type(obj, &lv_button_class))
            lv_obj_add_style(obj, &styleObjectBkg, 0);
        if (lv_obj_check_type(obj, &lv_switch_class))
        {
            lv_obj_add_style(obj, &styleObjectBkg, 0);
            lv_obj_add_style(obj, &styleObjectBkg, LV_PART_INDICATOR | LV_STATE_CHECKED);
        }
        if (lv_obj_check_type(obj, &lv_checkbox_class))
        {
            lv_obj_add_style(obj, &styleThemeBkg, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_add_style(obj, &styleObjectBkg, LV_PART_INDICATOR | LV_STATE_CHECKED);
        }
    }
}

/**
 * @brief Custom Dark theme
 *
 */
void modifyTheme()
{
    /*Initialize the styles*/
    lv_style_init(&styleThemeBkg);
    lv_style_set_bg_color(&styleThemeBkg, lv_color_black());
    lv_style_set_border_color(&styleThemeBkg, lv_color_hex(objectColor));
    lv_style_init(&styleObjectBkg);
    lv_style_set_bg_color(&styleObjectBkg, lv_color_hex(objectColor));
    lv_style_set_border_color(&styleObjectBkg, lv_color_hex(objectColor));
    lv_style_init(&styleObjectSel);
    lv_style_set_bg_color(&styleObjectSel, lv_color_hex(0x757575));

    /*Initialize the new theme from the current theme*/
    lv_theme_t *th_act = lv_disp_get_theme(NULL);
    static lv_theme_t th_new;
    th_new = *th_act;

    /*Set the parent theme and the style apply callback for the new theme*/
    lv_theme_set_parent(&th_new, th_act);
    lv_theme_set_apply_cb(&th_new, applyModifyTheme);

    /*Assign the new theme to the current display*/
    lv_disp_set_theme(NULL, &th_new);
}

/**
 * @brief Setting up tick task for lvgl
 * 
 * @param arg 
 */
void lv_tick_task(void *arg)
{
    (void)arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
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

    modifyTheme();

    //  Create Screens
    createSearchSatScr();
    createMainScr();
    createNotifyBar();
    createSettingsScr();
    createMapSettingsScr();
    createDeviceSettingsScr();
    createButtonBarScr();

    // Create and start a periodic timer interrupt to call lv_tick_inc 
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));
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