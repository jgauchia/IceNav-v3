/**
 * @file power.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  ESP32 Power Management functions
 * @version 0.1.5
 * @date 2023-06-04
 */

#include <WiFi.h>
#include <driver/rtc_io.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_wifi.h>

void powerDeepSeep()
{
  esp_bluedroid_disable();
  esp_bt_controller_disable();
  esp_wifi_stop();
  esp_deep_sleep_disable_rom_logging();
  delay(10);
  esp_deep_sleep_start();
}

void powerLightSleepTimer(int millis)
{
  esp_sleep_enable_timer_wakeup(millis * 1000);
  esp_err_t rtc_gpio_hold_en(gpio_num_t GPIO_NUM_5);
  esp_light_sleep_start();
}

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