/**
 * @file tft.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief TFT definition and functions
 * @version 0.1
 * @date 2022-10-10
 */

#include <TFT_eSPI.h>

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
}