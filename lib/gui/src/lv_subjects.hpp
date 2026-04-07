/**
 * @file lv_subjects.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL Observer Pattern - Subjects for telemetry data
 * @version 0.2.5
 * @date 2026-04
 */

#pragma once

#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/**
 * @brief Global subjects for reactive UI updates
 */
extern lv_subject_t subject_heading;
extern lv_subject_t subject_battery;
extern lv_subject_t subject_speed;
extern lv_subject_t subject_altitude;
extern lv_subject_t subject_lat;
extern lv_subject_t subject_lon;
extern lv_subject_t subject_time;
extern lv_subject_t subject_sats;
extern lv_subject_t subject_fix_mode;
extern lv_subject_t subject_is_fixed;
extern lv_subject_t subject_wifi;
extern lv_subject_t subject_map_state;
extern lv_subject_t subject_map_offset_x;
extern lv_subject_t subject_map_offset_y;

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

/**
 * @brief Initialize all telemetry subjects
 * 
 * @details Call this function during LVGL setup to prepare the subjects for observers.
 */
void init_lv_subjects();
