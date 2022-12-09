/**
 * @file tft.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief TFT definition and functions
 * @version 0.1
 * @date 2022-10-10
 */

#ifdef CUSTOMBOARD
#include "../lib/lovyangfx_CUSTOMBOARD_conf.h"
#include <LGFX_TFT_eSPI.hpp>
#else
#endif

int brightness_level = 255;

static TFT_eSPI tft;
static TFT_eSprite sat_sprite = TFT_eSprite(&tft);
static TFT_eSprite compass_sprite = TFT_eSprite(&tft);

/**
 * @brief Set the TFT brightness
 *
 * @param brightness -> 0..255
 */
void set_brightness(int brightness)
{
  if (brightness <= 255)
  {
    ledcWrite(0, brightness);
    brightness_level = brightness;
  }
}

/**
 * @brief Get the TFT brightness
 *
 * @return int -> brightness value 0..255
 */
int get_brightness()
{
  return brightness_level;
}

/**
 * @brief Turn on TFT Sleep Mode for ILI9488
 *
 */
void tft_on()
{
  tft.writecommand(0x11);
  set_brightness(255);
}

/**
 * @brief Turn off TFT Wake up Mode for ILI9488
 *
 */
void tft_off()
{
  tft.writecommand(0x10);
  set_brightness(0);
}

/**
 * @brief Touch calibrate
 *
 */
void touch_calibrate()
{
  uint16_t calData[8];
  uint8_t calDataOK = 0;

  if (SPIFFS.exists(CALIBRATION_FILE))
  {
    if (REPEAT_CAL)
      SPIFFS.remove(CALIBRATION_FILE);
    else
    {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f)
      {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL)
    tft.setTouchCalibrate(calData);
  else
  {
    tft.calibrateTouch(calData, TFT_WHITE, TFT_BLACK, std::max(tft.width(), tft.height()) >> 3);

    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f)
    {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}

/**
 * @brief Init tft display
 *
 */
void init_tft()
{
  tft.init();
#ifdef CUSTOMBOARD
  tft.setRotation(8);
#endif

#ifdef TDISPLAY
  tft.setRotation(8);
#endif

  tft.initDMA();
  tft.startWrite();
  tft.fillScreen(TFT_BLACK);
  tft.endWrite();
  
  gpio_set_drive_capability(GPIO_NUM_33, GPIO_DRIVE_CAP_3);
  ledcAttachPin(TFT_BL, 0);
  ledcSetup(0, 8000, 8);
  ledcWrite(0, 255);
  touch_calibrate();
}