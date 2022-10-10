/**
 * @file screens.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Screen includes
 * @version 0.1
 * @date 2022-10-10
 */

/**
 * @brief Notify Icons size
 * 
 */
#define Icon_Notify_Width 24
#define Icon_Notify_Height 24

/**
 * @brief Satellite signal bar size and memory buffer
 * 
 */
#define SNR_BAR_W 25
#define SNR_BAR_H 80
uint16_t snr_bkg[4428] = {0};

/**
 * @brief Map Zoom variables
 * 
 */
#define MIN_ZOOM 6
#define MAX_ZOOM 18
#define DEF_ZOOM 16
int zoom = DEF_ZOOM;
int zoom_old = 0;

/**
 * @brief Screen status flags
 * 
 */
bool is_draw = false;
bool is_menu_screen = false;
bool is_main_screen = false;
bool is_map_screen = false;
bool is_sat_screen = false;
bool is_compass_screen = false;
bool is_show_degree = true;

/**
 * @brief Vectors to a screen functions
 * 
 */
#define MAX_MAIN_SCREEN 3
typedef void (*MainScreenFunc)();
MainScreenFunc MainScreen[] = {0, show_main_screen, show_map_screen, show_sat_track_screen};
int sel_MainScreen = 1;

/**
 * @brief Screens
 * 
 */
#include "gui/screens/search_sat.h"
#include "gui/screens/splash.h"
#include "gui/widgets/sat_icon.h"
#include "gui/widgets/batt_icon.h"
#include "gui/widgets/compass.h"
#include "gui/widgets/map.h"
#include "gui/widgets/hour.h"
#include "gui/widgets/notify_bar.h"