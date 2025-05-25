/**
 * @file splashScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Splash screen - NOT LVGL
 * @version 0.2.2
 * @date 2025-05
 */

#include "splashScr.hpp"

static unsigned long millisActual = 0;
extern Maps mapView;
extern Gps gps;

/**
 * @brief Splash screen
 *
 */
void splashScreen()
{
  // Preload Map
  if (mapSet.vectorMap)
  {
    mapView.isPosMoved = true;
    mapView.generateVectorMap(zoom);
  }
  else
  {
    mapView.currentMapTile = mapView.getMapTile(gps.gpsData.longitude, gps.gpsData.latitude, zoom, 0, 0);
    mapView.generateRenderMap(zoom);
  }

  setTime = false;

  tftOff();
  
  TFT_eSprite splashSprite = TFT_eSprite(&tft);  
  splashSprite.createSprite(tft.width(), tft.height());  

  tft.fillScreen(TFT_BLACK);
  millisActual = millis();
  tft.setBrightness(0);

  static uint16_t pngHeight = 0;
  static uint16_t pngWidth = 0;

  getPngSize(logoFile, &pngWidth, &pngHeight);
  splashSprite.fillScreen(TFT_BLACK);
  splashSprite.drawPngFile(logoFile, (tft.width() / 2) - (pngWidth / 2), (tft.height() / 2) - pngHeight);

  splashSprite.setTextSize(1);
  splashSprite.setTextColor(TFT_WHITE, TFT_BLACK);

  splashSprite.drawCenterString("Map data from OpenStreetMap.", tft.width() >> 1, TFT_HEIGHT - 120);
  splashSprite.drawCenterString("(c) OpenStreetMap", tft.width() >> 1, TFT_HEIGHT - 110);
  splashSprite.drawCenterString("(c) OpenStreetMap contributors", tft.width() >> 1, TFT_HEIGHT - 100);

  char statusString[50] = "";
  splashSprite.setTextColor(TFT_YELLOW, TFT_BLACK);

  memset(&statusString[0], 0, sizeof(statusString));
  sprintf(statusString, statusLine1, ESP.getChipModel(), ESP.getCpuFreqMHz());
  splashSprite.drawString(statusString, 0, TFT_HEIGHT - 50);

  memset(&statusString[0], 0, sizeof(statusString));
  sprintf(statusString, statusLine2, (ESP.getFreeHeap() / 1024), (ESP.getFreeHeap() * 100) / ESP.getHeapSize());
  splashSprite.drawString(statusString, 0, TFT_HEIGHT - 40);

  memset(&statusString[0], 0, sizeof(statusString));
  sprintf(statusString, statusLine3, ESP.getPsramSize(), ESP.getPsramSize() - ESP.getFreePsram());
  splashSprite.drawString(statusString, 0, TFT_HEIGHT - 30);

  memset(&statusString[0], 0, sizeof(statusString));
  sprintf(statusString, statusLine4, String(VERSION), String(REVISION));
  splashSprite.drawString(statusString, 0, TFT_HEIGHT - 20);

  memset(&statusString[0], 0, sizeof(statusString));
  sprintf(statusString, statusLine5, String(FLAVOR));
  splashSprite.drawString(statusString, 0, TFT_HEIGHT - 10);

  memset(&statusString[0], 0, sizeof(statusString));
  splashSprite.setTextColor(TFT_WHITE, TFT_BLACK);
 
  const uint8_t maxBrightness = 255;

  tftOn(0);

  for (uint8_t fadeIn = 0; fadeIn <= (maxBrightness - 1); fadeIn++)
  {
    tft.setBrightness(fadeIn);
      if (fadeIn == 0)
    splashSprite.pushSprite(0,0);
    millisActual = millis();
    while (millis() < millisActual + 15);
  }

  millisActual = millis();
  while (millis() < millisActual + 100);

  for (uint8_t fadeOut = maxBrightness; fadeOut > 0; fadeOut--)
  {
    tft.setBrightness(fadeOut);
    millisActual = millis();
    while (millis() < millisActual + 15);
  }

  tft.fillScreen(TFT_BLACK);

  while (millis() < millisActual + 100);

  tft.setBrightness(defBright);
  
  splashSprite.deleteSprite();
}
