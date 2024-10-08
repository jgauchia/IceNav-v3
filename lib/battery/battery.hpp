/**
 * @file battery.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Battery monitor definition and functions
 * @version 0.1.8_Alpha
 * @date 2024-09
 */

#ifndef BATTERY_HPP
#define BATTERY_HPP

#include <Arduino.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

static esp_adc_cal_characteristics_t characteristics;
#define V_REF 3.9 // ADC reference voltage

static float batteryMax = 4.20;     // 4.2;      // maximum voltage of battery
static float batteryMin = 3.40;     // 3.6;      // minimum voltage of battery before shutdown

extern uint8_t battLevel;
extern uint8_t battLevelOld;

void initADC();
float batteryRead();

#endif
