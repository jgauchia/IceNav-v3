/**
 * @file tasks.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Core Tasks functions
 * @version 0.1.8
 * @date 2024-05
 */

#include "tasks.hpp"

time_t local, utc = 0;

/**
 * @brief Task1 - LVGL Task
 *
 * @param pvParameters
 */
void lvglTask(void *pvParameters)
{
    log_v("Task1 - LVGL Task - running on core %d", xPortGetCoreID());
    log_v("Stack size: %d", uxTaskGetStackHighWaterMark(NULL));
    for (;;)
    {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(TASK_SLEEP_PERIOD_MS));
    }
}

/**
 * @brief Init Task1 LVGL Task
 * 
 */
void initLvglTask()
{
    xTaskCreatePinnedToCore(lvglTask, PSTR("LVGL Task"), 20000, NULL, 2, NULL, 1);
    delay(500);
}

/**
 * @brief Task 2 - Read GPS data
 *
 * @param pvParameters
 */
void gpsTask(void *pvParameters)
{
    log_v("Task 2 - GPS Task - running on core %d", xPortGetCoreID());
    log_v("Stack size: %d", uxTaskGetStackHighWaterMark(NULL));
    for (;;)
    {
        while (gps->available() > 0)
        {
            #ifdef OUTPUT_NMEA
            {
                Serial.write(gps->read());
            }
            #else
            GPS.encode(gps->read());
            #endif
        }
        if (GPS.time.isValid() && !isTimeFixed)
        {
            setTime(GPS.time.hour(),
                    GPS.time.minute(),
                    GPS.time.second(),
                    GPS.date.day(),
                    GPS.date.month(),
                    GPS.date.year());
            utc = now();
            local = CE.toLocal(utc);
            setTime(local);
            isTimeFixed = true;
        }
        // if (!GPS.time.isValid() && isTimeFixed)
        //     isTimeFixed = false;

        vTaskDelay(10);

    }
}

/**
 * @brief Init Task 2 GPS task
 *
 */
void initGpsTask()
{
    xTaskCreatePinnedToCore(gpsTask, PSTR("GPS Task"), 8192, NULL, 1, NULL, 1);
    delay(500);
}

