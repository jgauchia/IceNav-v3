/**
 * @file lv_subjects.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL Observer Pattern - Implementation of telemetry subjects
 * @version 0.2.9
 * @date 2026-06
 */

#include "lv_subjects.hpp"
#include "gps.hpp"
#include "bme.hpp"
#include "settings.hpp"
#include <time.h>

lv_subject_t subject_heading;
lv_subject_t subject_compass_heading;
lv_subject_t subject_battery;
lv_subject_t subject_speed;
lv_subject_t subject_altitude;
lv_subject_t subject_lat;
lv_subject_t subject_lon;
lv_subject_t subject_time;
lv_subject_t subject_sats;
lv_subject_t subject_pdop;
lv_subject_t subject_hdop;
lv_subject_t subject_vdop;
lv_subject_t subject_sats_data_trigger;
lv_subject_t subject_fix_mode;
lv_subject_t subject_is_fixed;
lv_subject_t subject_wifi;
lv_subject_t subject_map_state;
lv_subject_t subject_map_offset_x;
lv_subject_t subject_map_offset_y;
lv_subject_t subject_sunrise;
lv_subject_t subject_rerouting;
lv_subject_t subject_climb_active;
lv_subject_t subject_climb_dist;
lv_subject_t subject_climb_gain;
lv_subject_t subject_climb_grade;
lv_subject_t subject_climb_idx;
lv_subject_t subject_climb_seg;
lv_subject_t subject_climb_total;
lv_subject_t subject_climb_cat;
lv_subject_t subject_climb_avg_grade;
lv_subject_t subject_climb_total_dist;
lv_subject_t subject_climb_total_gain;
lv_subject_t subject_climb_approaching;
lv_subject_t subject_map_3d;
lv_subject_t subject_nmea_debug_trigger;

#ifdef ENABLE_TEMP
lv_subject_t subject_temp;
#endif

volatile bool canMoveWidget = false;

/**
 * @brief Initialize all telemetry subjects
 * 
 * @details Initializes the subjects with default values for integers.
 */
void init_lv_subjects()
{
    lv_subject_init_int(&subject_heading, 0);
    lv_subject_init_int(&subject_compass_heading, 0);
    lv_subject_init_int(&subject_battery, 0);
    lv_subject_init_int(&subject_speed, 0);
    lv_subject_init_int(&subject_altitude, 0);
    lv_subject_init_int(&subject_lat, 0);
    lv_subject_init_int(&subject_lon, 0);
    lv_subject_init_int(&subject_time, 0);
    lv_subject_init_int(&subject_sats, 0);
    lv_subject_init_int(&subject_pdop, 0);
    lv_subject_init_int(&subject_hdop, 0);
    lv_subject_init_int(&subject_vdop, 0);
    lv_subject_init_int(&subject_sats_data_trigger, 0);
    lv_subject_init_int(&subject_fix_mode, 0);
    lv_subject_init_int(&subject_is_fixed, 0);
    lv_subject_init_int(&subject_wifi, 0);
    lv_subject_init_int(&subject_map_state, 0);
    lv_subject_init_int(&subject_map_offset_x, 0);
    lv_subject_init_int(&subject_map_offset_y, 0);
    lv_subject_init_int(&subject_sunrise, 0);
    lv_subject_init_int(&subject_rerouting, 0);
    lv_subject_init_int(&subject_climb_active, 0);
    lv_subject_init_int(&subject_climb_dist, 0);
    lv_subject_init_int(&subject_climb_gain, 0);
    lv_subject_init_int(&subject_climb_grade, 0);
    lv_subject_init_int(&subject_climb_idx,         0);
    lv_subject_init_int(&subject_climb_seg,         0);
    lv_subject_init_int(&subject_climb_total,       0);
    lv_subject_init_int(&subject_climb_cat,         0);
    lv_subject_init_int(&subject_climb_avg_grade,   0);
    lv_subject_init_int(&subject_climb_total_dist,  0);
    lv_subject_init_int(&subject_climb_total_gain,  0);
    lv_subject_init_int(&subject_climb_approaching, 0);
    lv_subject_init_int(&subject_map_3d, 0);
    lv_subject_init_int(&subject_nmea_debug_trigger, 0);

    #ifdef ENABLE_TEMP
    {
        #ifdef BME280
        float t = 0.0f;
        float p = 0.0f;
        float h = 0.0f;
        bme.readAll(t, p, h);
        lv_subject_init_int(&subject_temp, (int32_t)(t + tempOffset));
        #else
        lv_subject_init_int(&subject_temp, 0);
        #endif
    }
    #endif
}

/**
 * @brief Manually trigger all telemetry observers
 * 
 * @details Forces a refresh of all reactive UI components by notifying their subjects.
 */
void notify_all_subjects()
{
    lv_subject_notify(&subject_heading);
    lv_subject_notify(&subject_compass_heading);
    lv_subject_notify(&subject_battery);
    lv_subject_notify(&subject_speed);
    lv_subject_notify(&subject_altitude);
    lv_subject_notify(&subject_lat);
    lv_subject_notify(&subject_lon);
    lv_subject_notify(&subject_time);
    lv_subject_notify(&subject_sats);
    lv_subject_notify(&subject_pdop);
    lv_subject_notify(&subject_hdop);
    lv_subject_notify(&subject_vdop);
    lv_subject_notify(&subject_sats_data_trigger);
    lv_subject_notify(&subject_fix_mode);
    lv_subject_notify(&subject_is_fixed);
    lv_subject_notify(&subject_wifi);
    lv_subject_notify(&subject_map_state);
    lv_subject_notify(&subject_map_offset_x);
    lv_subject_notify(&subject_map_offset_y);
    lv_subject_notify(&subject_sunrise);
    lv_subject_notify(&subject_climb_active);
    lv_subject_notify(&subject_climb_dist);
    lv_subject_notify(&subject_climb_gain);
    lv_subject_notify(&subject_climb_grade);

    #ifdef ENABLE_TEMP
    lv_subject_notify(&subject_temp);
    #endif
}
