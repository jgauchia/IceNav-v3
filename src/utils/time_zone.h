/**
 * @file time_zone.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Timezone Library (Daylight Saving)
 * @version 0.1.8
 * @date 2024-04
 */

/**
 * @brief Central European Time (daylight saving time)
 * 
 */
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};
Timezone CE(CEST, CET);

/**
 * @brief Time Variables
 * 
 */
time_t local, utc;