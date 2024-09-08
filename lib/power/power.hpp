/**
 * @file power.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  ESP32 Power Management functions
 * @version 0.1.8_Alpha
 * @date 2024-09
 */

#ifndef POWER_HPP
#define POWER_HPP

#include <WiFi.h>
#include <driver/rtc_io.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_wifi.h>

void powerDeepSeep();
void powerLightSleepTimer(int millis);
void powerOn();

#endif