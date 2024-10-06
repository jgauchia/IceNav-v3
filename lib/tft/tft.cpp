/**
 * @file tft.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief TFT definition and functions
 * @version 0.1.8_Alpha
 * @date 2024-09
 */

#include "tft.hpp"

TFT_eSPI tft = TFT_eSPI();
bool repeatCalib = false;
uint16_t TFT_WIDTH = 0;
uint16_t TFT_HEIGHT = 0;
bool waitScreenRefresh = false;

#ifdef TDECK_ESP32S3 
  extern const uint8_t TFT_SPI_BL;
#endif

/**
 * @brief Set the TFT brightness
 *
 * @param brightness -> 0..255 / 0..15 for T-DECK
 */
void setBrightness(uint8_t brightness)
{
  
  #ifndef TDECK_ESP32S3 
  if (brightness <= 255)
  {
   ledcWrite(0, brightness);
   brightnessLevel = brightness;
  }
  #endif

  #ifdef TDECK_ESP32S3 
    static uint8_t level = 0;
    static uint8_t steps = 16;
    if (brightness == 0) 
    {
      digitalWrite(TFT_SPI_BL, 0);
      delay(3);
      level = 0;
      return;
    }
    if (level == 0) 
    {
      digitalWrite(TFT_SPI_BL, 1);
      level = steps;
      delayMicroseconds(30);
    }
    int from = steps - level;
    int to = steps - brightness;
    int num = (steps + to - from) % steps;
    for (int i = 0; i < num; i++) 
    {
      digitalWrite(TFT_SPI_BL, 0);
      digitalWrite(TFT_SPI_BL, 1);
    }
    level = brightness;
    brightnessLevel = brightness;
  #endif
}

/**
 * @brief Get the TFT brightness
 *
 * @return int -> brightness value 0..255 / 0..15 for T-DECK
 */
uint8_t getBrightness()
{
  return brightnessLevel;
}

/**
 * @brief Turn on TFT Sleep Mode for ILI9488
 *
 */
void tftOn(uint8_t brightness)
{
  tft.writecommand(0x11);
  delay(120);
  tft.setBrightness(brightness);
}

/**
 * @brief Turn off TFT Wake up Mode for ILI9488
 *
 */
void tftOff()
{
  tft.setBrightness(0);
  tft.writecommand(0x10);
}

/**
 * @brief Touch calibrate
 *
 */
void touchCalibrate()
{
  uint16_t calData[8];
  uint8_t calDataOK = 0;

  FILE* f = fopen(calibrationFile, "r");

  if (f != NULL)
  {
    if (repeatCalib)
      remove(calibrationFile);
    else
    {
      if (fread((char *)calData, sizeof(char), 16, f))
      {
        log_i("Touch calibration exists");
        calDataOK = 1;
        fclose(f);
      }
    }
  }
  else
    log_e("Touch calibration doesn't exists");

  if (calDataOK && !repeatCalib)
    tft.setTouchCalibrate(calData);
  else
  {
    static const lgfx::v1::GFXfont* fontSmall;
    static const lgfx::v1::GFXfont* fontLarge;

    #ifdef LARGE_SCREEN
      fontSmall = &fonts::DejaVu18;
      fontLarge = &fonts::DejaVu40;
    #else
      fontSmall = &fonts::DejaVu12;
      fontLarge = &fonts::DejaVu24;
    #endif

    tft.drawCenterString("TOUCH THE ARROW MARKER.", tft.width() >> 1, tft.height() >> 1, fontSmall);
    tft.calibrateTouch(calData, TFT_WHITE, TFT_BLACK, std::max(tft.width(), tft.height()) >> 3);
    tft.drawCenterString("DONE!", tft.width() >> 1, (tft.height() >> 1) + (tft.fontHeight(fontSmall) * 2), fontLarge);
    delay(500);
    tft.drawCenterString("TOUCH TO CONTINUE.", tft.width() >> 1, (tft.height() >> 1) + (tft.fontHeight(fontLarge) * 2), fontSmall);

    FILE* f = fopen(calibrationFile, "w");
    if (f)
    {
      log_v("Calibration saved");
      fwrite((const unsigned char *)calData, sizeof(unsigned char), 16 ,f);
      fclose(f);
    }
    else
      log_e("Calibration not saved!");

    uint16_t touchX, touchY;
    while (!tft.getTouch(&touchX, &touchY));
  }
}

/**
 * @brief Init TFT display
 *
 */
void initTFT()
{
  tft.init();
  
  #ifdef TDECK_ESP32S3
    tft.setRotation(1);
  #endif

  TFT_HEIGHT = tft.height();
  TFT_WIDTH = tft.width();

  tft.initDMA();
  tft.startWrite();
  tft.fillScreen(TFT_BLACK);
  tft.endWrite();

#ifdef ICENAV_BOARD
  gpio_set_drive_capability(GPIO_NUM_45, GPIO_DRIVE_CAP_3);
#endif
#ifdef ESP32_N16R4
  gpio_set_drive_capability(GPIO_NUM_33, GPIO_DRIVE_CAP_3);
#endif
#ifdef ESP32S3_N16R8
  gpio_set_drive_capability(GPIO_NUM_45, GPIO_DRIVE_CAP_3);
#endif
#ifdef MAKERF_ESP32S3
  gpio_set_drive_capability(GPIO_NUM_45, GPIO_DRIVE_CAP_3);
#endif
#ifdef ELECROW_ESP32
  gpio_set_drive_capability(GPIO_NUM_46, GPIO_DRIVE_CAP_3);
#endif

#ifndef TDECK_ESP32S3
  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, 255);
#endif

#ifdef TOUCH_INPUT
  touchCalibrate();
#endif
}
