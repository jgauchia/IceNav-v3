/**
 * @file power.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  ESP32 Power Management functions
 * @version 0.1.9
 * @date 2024-12
 */

#ifndef POWER_HPP
#define POWER_HPP

#include <Wire.h>
#include <WiFi.h>
#include <driver/rtc_io.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_wifi.h>
#include "tft.hpp"
#include "lvgl.h"
#include "globalGuiDef.h"

class Power
{
private:
    void powerDeepSleep();
    void powerLightSleepTimer(int millis);
    void powerLightSleep();
    void powerOffPeripherals();

public:
    Power();

    void deviceSuspend();
    void deviceShutdown();
};


#endif