/**
 * @file splashScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Splash screen - NOT LVGL
 * @version 0.2.0
 * @date 2025-04
 */

#include "splashScr.hpp"

static unsigned long millisActual = 0;
extern Maps mapView;

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
    mapView.generateRenderMap(zoom);

  setTime = false;

  tft.fillScreen(TFT_BLACK);
  millisActual = millis();
  tft.setBrightness(0);

  static uint16_t pngHeight = 0;
  static uint16_t pngWidth = 0;

  getPngSize(logoFile, &pngWidth, &pngHeight);
  tft.drawPngFile(logoFile, (tft.width() / 2) - (pngWidth / 2), (tft.height() / 2) - pngHeight);

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.drawCenterString("Map data from OpenStreetMap.", tft.width() >> 1, TFT_HEIGHT - 120);
  tft.drawCenterString("(c) OpenStreetMap", tft.width() >> 1, TFT_HEIGHT - 110);
  tft.drawCenterString("(c) OpenStreetMap contributors", tft.width() >> 1, TFT_HEIGHT - 100);

  char statusString[50] = "";
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

  memset(&statusString[0], 0, sizeof(statusString));
  sprintf(statusString, statusLine1, ESP.getChipModel(), ESP.getCpuFreqMHz());
  tft.drawString(statusString, 0, TFT_HEIGHT - 50);

  memset(&statusString[0], 0, sizeof(statusString));
  sprintf(statusString, statusLine2, (ESP.getFreeHeap() / 1024), (ESP.getFreeHeap() * 100) / ESP.getHeapSize());
  tft.drawString(statusString, 0, TFT_HEIGHT - 40);

  memset(&statusString[0], 0, sizeof(statusString));
  sprintf(statusString, statusLine3, ESP.getPsramSize(), ESP.getPsramSize() - ESP.getFreePsram());
  tft.drawString(statusString, 0, TFT_HEIGHT - 30);

  memset(&statusString[0], 0, sizeof(statusString));
  sprintf(statusString, statusLine4, String(VERSION), String(REVISION));
  tft.drawString(statusString, 0, TFT_HEIGHT - 20);

  memset(&statusString[0], 0, sizeof(statusString));
  sprintf(statusString, statusLine5, String(FLAVOR));
  tft.drawString(statusString, 0, TFT_HEIGHT - 10);

  memset(&statusString[0], 0, sizeof(statusString));
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  const uint8_t maxBrightness = 255;

  for (uint8_t fadeIn = 0; fadeIn <= (maxBrightness - 1); fadeIn++)
  {
    tft.setBrightness(fadeIn);
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
}
