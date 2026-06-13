/**
 * @file lv_subjects.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL Observer Pattern - Subjects for telemetry data
 * @version 0.2.9
 * @date 2026-06
 */

#pragma once

#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/**
 * @brief Global subjects for reactive UI updates
 */
extern lv_subject_t subject_heading;
extern lv_subject_t subject_compass_heading;
extern lv_subject_t subject_battery;
extern lv_subject_t subject_speed;
extern lv_subject_t subject_altitude;
extern lv_subject_t subject_lat;
extern lv_subject_t subject_lon;
extern lv_subject_t subject_time;
extern lv_subject_t subject_sats;
extern lv_subject_t subject_pdop;
extern lv_subject_t subject_hdop;
extern lv_subject_t subject_vdop;
extern lv_subject_t subject_sats_data_trigger;
extern lv_subject_t subject_fix_mode;
extern lv_subject_t subject_is_fixed;
extern lv_subject_t subject_wifi;
extern lv_subject_t subject_map_state;
extern lv_subject_t subject_map_offset_x;
extern lv_subject_t subject_map_offset_y;
extern lv_subject_t subject_sunrise;
extern lv_subject_t subject_rerouting;    // 0=normal, 1=calculating route
extern lv_subject_t subject_climb_active; // 0=hidden, 1=visible
extern lv_subject_t subject_climb_dist;   // remaining climb distance (meters, int)
extern lv_subject_t subject_climb_gain;   // remaining elevation gain (meters, int)
extern lv_subject_t subject_climb_grade;  // current grade * 10 (int, e.g. 35 = 3.5%)
extern lv_subject_t subject_climb_idx;         // nearest trackData index for canvas redraw
extern lv_subject_t subject_climb_seg;         // 1-based active segment index
extern lv_subject_t subject_climb_total;       // total number of segments in track
extern lv_subject_t subject_climb_cat;         // category: 1=HC,2=CAT1,...,5=CAT4,0=none
extern lv_subject_t subject_climb_avg_grade;   // avgGrade * 10 (int)
extern lv_subject_t subject_climb_total_dist;  // totalDist of segment (meters, int)
extern lv_subject_t subject_climb_total_gain;  // totalGain of segment (meters, int)
extern lv_subject_t subject_climb_approaching; // 0=inside climb, 1=approaching start
extern lv_subject_t subject_map_3d;            // 0=2D view, 1=pseudo-3D view
extern lv_subject_t subject_nmea_debug_trigger; // increments each GPS cycle (debug tile)

#ifdef ENABLE_TEMP
extern lv_subject_t subject_temp;
#endif

/**
 * @brief Mutex for thread-safe LVGL access across cores
 */
extern SemaphoreHandle_t lvgl_mutex;

/**
 * @brief Flag to indicate if a widget is currently being dragged/moved
 */
extern volatile bool canMoveWidget;

void init_lv_subjects();

void notify_all_subjects();
