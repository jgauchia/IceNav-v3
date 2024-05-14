/**
 * @file splashScr.hpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Splash screen - NOT LVGL
 * @version 0.1.8
 * @date 2024-05
 */

#include "splashScr.hpp"

static unsigned long millisActual = 0;

/**
 * @brief Splash screen
 *
 */
void splashScreen()
{
    tft.fillScreen(TFT_BLACK);
    millisActual = millis();
    setBrightness(0);
    tft.drawPngFile(SPIFFS, logoFile, (tft.width() / 2) - 150, (tft.height() / 2) - 70);
    char statusString[100] = "";
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    memset(&statusString[0], 0, sizeof(statusString));
    sprintf(statusString, statusLine1, ESP.getChipModel(), ESP.getCpuFreqMHz(), (ESP.getFreeHeap() / 1024), (ESP.getFreeHeap() * 100) / ESP.getHeapSize());
    tft.drawString(statusString, 10, 450);
    memset(&statusString[0], 0, sizeof(statusString));
    sprintf(statusString, statusLine2, ESP.getPsramSize(), ESP.getPsramSize() - ESP.getFreePsram());
    tft.drawString(statusString, 10, 460);
    memset(&statusString[0], 0, sizeof(statusString));
    sprintf(statusString, statusLine3, String(VERSION), String(REVISION), String(FLAVOR));
    tft.drawString(statusString, 10, 470);
    memset(&statusString[0], 0, sizeof(statusString));
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    for (uint8_t fadeIn = 0; fadeIn <= UINT8_MAX - 1; fadeIn++)
    {
        setBrightness(fadeIn);
        millisActual = millis();
        while (millis() < millisActual + 15)
            ;
    }
    for (uint8_t fadeOut = UINT8_MAX; fadeOut > 0; fadeOut--)
    {
        setBrightness(fadeOut);
        millisActual = millis();
        while (millis() < millisActual + 15)
            ;
    }
    while (millis() < millisActual + 100)
        ;

    tft.fillScreen(TFT_BLACK);
    setBrightness(255);
}
