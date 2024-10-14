/**
 * @file power.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  ESP32 Power Management functions
 * @version 0.1.8_Alpha
 * @date 2024-10
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

void powerDeepSeep();
void powerLightSleepTimer(int millis);
void powerLightSleep();
void powerOffPeripherals();
void deviceSuspend();
void deviceShutdown();
void powerOn();

#endif