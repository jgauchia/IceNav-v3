/**
 * @file power.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  ESP32 Power Management functions
 * @version 0.1.8
 * @date 2024-05
 */

#include "power.hpp"

/**
 * @brief Deep Sleep Mode
 * 
 */
void powerDeepSeep()
{
  esp_bluedroid_disable();
  esp_bt_controller_disable();
  esp_wifi_stop();
  esp_deep_sleep_disable_rom_logging();
  delay(10);
  esp_deep_sleep_start();
}

/**
 * @brief Sleep Mode Timer
 * 
 * @param millis 
 */
void powerLightSleepTimer(int millis)
{
  esp_sleep_enable_timer_wakeup(millis * 1000);
  esp_err_t rtc_gpio_hold_en(gpio_num_t GPIO_NUM_5);
  esp_light_sleep_start();
}

/**
 * @brief On Mode
 * 
 */
void powerOn()
{
#ifdef DISABLE_RADIO
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();
  esp_wifi_stop();
  esp_bt_controller_disable();
#endif
}
