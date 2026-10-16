/**
 * @file lvglSubjects.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL Observer Pattern - Subjects for telemetry data
 * @version 0.3.3
 * @date 2026-10
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
 * @brief Default wait budget for lvglSetSubjectInt()
 */
static constexpr uint32_t LVGL_SUBJECT_SET_TIMEOUT_MS = 100;

/**
 * @brief Set an integer subject serialising the observer callbacks with lvgl_mutex.
 *
 * @details lv_subject_set_int() runs the registered observers synchronously in the calling
 *          task, and those observers touch LVGL objects (labels, flags, canvases) that the GUI
 *          task may be rendering at the same time. Use this helper instead of
 *          lv_subject_set_int() from any task other than the GUI task; when the caller already
 *          holds lvgl_mutex the value is applied directly, so it never self-deadlocks on the
 *          non-recursive mutex.
 *
 * @param subject    Subject to update.
 * @param value      New integer value.
 * @param timeout_ms Wait budget for lvgl_mutex; pass 0 from tasks that must never wait on the
 *                   GUI task, so the update is skipped instead of stalling the sensor loop.
 * @return true when the value was applied; false when lvgl_mutex could not be taken and the
 *         update was dropped.
 */
bool lvglSetSubjectInt(lv_subject_t *subject, int32_t value,
                       uint32_t timeout_ms = LVGL_SUBJECT_SET_TIMEOUT_MS);

/**
 * @brief Flag to indicate if a widget is currently being dragged/moved
 */
extern volatile bool canMoveWidget;

void initLvglSubjects();

void notifyAllSubjects();
