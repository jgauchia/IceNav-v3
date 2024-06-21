/**
 * @file tft.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief TFT definition and functions
 * @version 0.1.8
 * @date 2024-06
 */

#include "tft.hpp"

TFT_eSPI tft = TFT_eSPI();
bool repeatCalib = false;
uint16_t TFT_WIDTH = 0;
uint16_t TFT_HEIGHT = 0;

/**
 * @brief Set the TFT brightness
 *
 * @param brightness -> 0..255
 */
void setBrightness(uint8_t brightness)
{
  if (brightness <= 255)
  {
    ledcWrite(0, brightness);
    brightnessLevel = brightness;
  }
}

/**
 * @brief Get the TFT brightness
 *
 * @return int -> brightness value 0..255
 */
uint8_t getBrightness()
{
  return brightnessLevel;
}

/**
 * @brief Turn on TFT Sleep Mode for ILI9488
 *
 */
void tftOn()
{
  tft.writecommand(0x11);
  setBrightness(255);
}

/**
 * @brief Turn off TFT Wake up Mode for ILI9488
 *
 */
void tftOff()
{
  tft.writecommand(0x10);
  setBrightness(0);
}

/**
 * @brief Touch calibrate
 *
 */
void touchCalibrate()
{
  uint16_t calData[8];
  uint8_t calDataOK = 0;

  if (SPIFFS.exists(calibrationFile))
  {
    if (repeatCalib)
      SPIFFS.remove(calibrationFile);
    else
    {
      File f = SPIFFS.open(calibrationFile, "r");
      if (f)
      {
        if (f.readBytes((char *)calData, 16) == 16)
          calDataOK = 1;
        f.close();
      }
      else
        log_v("Error opening touch configuration");
    }
  }

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

    File f = SPIFFS.open(calibrationFile, "w");
    if (f)
    {
      log_v("Calibration saved");
      f.write((const unsigned char *)calData, 16);
      f.close();
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

  TFT_HEIGHT = tft.height();
  TFT_WIDTH = tft.width();

  tft.initDMA();
  tft.startWrite();
  tft.fillScreen(TFT_BLACK);
  tft.endWrite();

#ifdef ARDUINO_ESP32_DEV
  gpio_set_drive_capability(GPIO_NUM_33, GPIO_DRIVE_CAP_3);
#endif
#ifdef ARDUINO_ESP32S3_DEV
  gpio_set_drive_capability(GPIO_NUM_45, GPIO_DRIVE_CAP_3);
#endif

  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, 255);
  touchCalibrate();
}
