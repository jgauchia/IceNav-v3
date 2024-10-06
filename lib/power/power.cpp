/**
 * @file power.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  ESP32 Power Management functions
 * @version 0.1.8_Alpha
 * @date 2024-09
 */

#include "power.hpp"

extern const uint8_t BOARD_BOOT_PIN;

/**
 * @brief Deep Sleep Mode
 * 
 */
void powerDeepSleep()
{
  esp_bluedroid_disable();
  esp_bt_controller_disable();
  esp_wifi_stop();
  esp_deep_sleep_disable_rom_logging();
  delay(10);
  // If you need other peripherals to maintain power, please set the IO port to hold
  // gpio_hold_en((gpio_num_t)BOARD_POWERON);
  // gpio_deep_sleep_hold_en();
  esp_sleep_enable_ext1_wakeup(1ull << BOARD_BOOT_PIN, ESP_EXT1_WAKEUP_ANY_LOW);
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
 * @brief Sleep Mode
 * 
 */
void powerLightSleep()
{
  esp_sleep_enable_ext1_wakeup(1ull << BOARD_BOOT_PIN, ESP_EXT1_WAKEUP_ANY_LOW);
  esp_light_sleep_start();
}

/**
 * @brief Core light suspend and TFT off
 */
void deviceSuspend()
{
  int brightness = tft.getBrightness();
  tftOff();
  powerLightSleep();
  tftOn(brightness);
  while (digitalRead(BOARD_BOOT_PIN) != 1)
  { 
    delay(5);
  };
  log_v("Exited sleep mode");
}

/**
 * @brief Power off peripherals and deep sleep
 */
void deviceShutdown()
{
  powerOffPeripherals();
  powerDeepSleep();
}

/**
 * @brief Power off peripherals devices
 */
void powerOffPeripherals()
{
  tftOff();
  #ifdef TDECK_ESP32S3
    SPI.end();
  #endif
  Wire.end();
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
