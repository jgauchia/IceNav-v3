/**
 * @file tft.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief TFT definition and functions
 * @version 0.3.0
 * @date 2026-06
 */

#include "tft.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

static const char *TAG = "TFT";

TFT_eSPI tft = TFT_eSPI();
bool repeatCalib = false;
uint16_t TFT_WIDTH = 0;
uint16_t TFT_HEIGHT = 0;
bool waitScreenRefresh = false;
extern Storage storage;

/**
 * @brief Turn on TFT (Wake up Mode) for ILI9488
 */
void tftOn(uint8_t brightness)
{
    tft.writecommand(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));
    tft.setBrightness(brightness);
}

/**
 * @brief Turn off TFT (Sleep Mode) for ILI9488
 */
void tftOff()
{
    tft.setBrightness(0);
    tft.writecommand(0x10);
}

/**
 * @brief Touch calibrate
 */
void touchCalibrate()
{
    uint16_t calData[8];
    uint8_t calDataOK = 0;

    FILE* f = storage.open(calibrationFile, "r");

    if (f != NULL)
    {
        if (repeatCalib)
            remove(calibrationFile);
        else
            if (storage.read(f, (char *)calData, 16))
            {
                calDataOK = 1;
                storage.close(f);
            }
    }
    else
        ESP_LOGE(TAG, "Touch calibration doesn't exists");

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

        TFT_eSprite touchSprite = TFT_eSprite(&tft);  
        touchSprite.createSprite(tft.width(), tft.height());  

        touchSprite.drawCenterString("TOUCH THE ARROW MARKER.", tft.width() >> 1, tft.height() >> 1, fontSmall);
        touchSprite.pushSprite(0,0);

        tft.calibrateTouch(calData, TFT_WHITE, TFT_BLACK, std::max(tft.width(), tft.height()) >> 3);
        touchSprite.drawCenterString("DONE!", tft.width() >> 1, (tft.height() >> 1) + (tft.fontHeight(fontSmall) * 2), fontLarge);
        touchSprite.pushSprite(0,0);
        vTaskDelay(pdMS_TO_TICKS(500));
        touchSprite.drawCenterString("TOUCH TO CONTINUE.", tft.width() >> 1, (tft.height() >> 1) + (tft.fontHeight(fontLarge) * 2), fontSmall);
        touchSprite.pushSprite(0,0);

        FILE* f = storage.open(calibrationFile, "w");
        if (f)
        {
            ESP_LOGV(TAG, "Calibration saved");
            fwrite((const unsigned char *)calData, sizeof(unsigned char), 16 ,f);
            storage.close(f);
        }
        else
            ESP_LOGE(TAG, "Calibration not saved!");

        uint16_t touchX;
        uint16_t touchY;
        while (!tft.getTouch(&touchX, &touchY));

        touchSprite.deleteSprite();
    }
}

/**
 * @brief Init TFT display
 */
void initTFT()
{
    tft.init();

    #ifdef T4_S3
    //   tft.enableFrameBuffer(false);
    #endif

    #ifdef TDECK_ESP32S3
        tft.setRotation(1);
    #endif

    TFT_HEIGHT = tft.height();
    TFT_WIDTH = tft.width();

    tft.initDMA();
    tft.fillScreen(TFT_BLACK);

    // The P4 board uses a capacitive controller (FT6336), which reports
    // already-calibrated coordinates, so the resistive calibration flow is
    // skipped here for now.
    #if defined(TOUCH_INPUT) && !CONFIG_IDF_TARGET_ESP32P4
        touchCalibrate();
    #endif
}
