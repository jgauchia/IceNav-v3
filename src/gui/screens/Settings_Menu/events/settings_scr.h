/**
 * @file settings_scr.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Settings Screen events
 * @version 0.1.8
 * @date 2024-04
 */

void loadMainScreen();

/**
 * @brief Back button event
 *
 * @param event
 */
static void back(lv_event_t *event)
{
    loadMainScreen();
}

/**
 * @brief Touch Calibration
 *
 * @param event
 */
static void touchCalib(lv_event_t *event)
{
    repeatCalib = true;
    tft.fillScreen(TFT_BLACK);
    touchCalibrate();
    repeatCalib = false;
    isMainScreen = false;
    tft.fillScreen(TFT_BLACK);
    lv_screen_load(settingsScreen);
}

/**
 * @brief Compass Calibration
 *
 * @param event
 */
static void compassCalib(lv_event_t *event)
{
    tft.fillScreen(TFT_BLACK);
    compassCalibrate();
    isMainScreen = false;
    tft.fillScreen(TFT_BLACK);
    lv_screen_load(settingsScreen);
}

/**
 * @brief Map Setting
 *
 * @param event
 */
static void mapSettings(lv_event_t *event)
{
    lv_screen_load(mapSettingsScreen);
}

/**
 * @brief Device Settings
 *
 * @param event
 */
static void deviceSettings(lv_event_t *event)
{
    lv_screen_load(deviceSettingsScreen);
}