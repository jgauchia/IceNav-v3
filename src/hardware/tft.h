/**
 * @file tft.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief TFT definition and functions
 * @version 0.1
 * @date 2022-10-10
 */

#include <TFT_eSPI.h>

int brightness_level = 255;

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sat_sprite = TFT_eSprite(&tft);
TFT_eSprite compass_sprite = TFT_eSprite(&tft);

/**
 * @brief Init tft display
 *
 */
void init_tft()
{
  tft.init();
#ifdef CUSTOMBOARD
  tft.setRotation(2);
#endif

#ifdef TDISPLAY
  tft.setRotation(4);
#endif

  tft.fillScreen(TFT_BLACK);
  tft.initDMA();
  gpio_set_drive_capability(GPIO_NUM_33, GPIO_DRIVE_CAP_3);
  ledcAttachPin(TFT_BL, 0);
  ledcSetup(0, 8000, 8);
  ledcWrite(0, 255);
}

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
 * @brief Turn on TFT Sleep Mode for ILI9341
 *
 */
void tft_on()
{
  tft.writecommand(0x11);
  set_brightness(255);
}

/**
 * @brief Turn off TFT Wake up Mode for ILI9341
 *
 */
void tft_off()
{
  tft.writecommand(0x10);
  set_brightness(0);
}