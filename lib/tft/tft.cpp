/**
 * @file tft.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief TFT definition and functions
 * @version 0.2.4
 * @date 2025-12
 */

#include "tft.hpp"

TFT_eSPI tft = TFT_eSPI();
bool repeatCalib = false;
uint16_t TFT_WIDTH = 0;
uint16_t TFT_HEIGHT = 0;
bool waitScreenRefresh = false;
extern Storage storage;

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

    FILE* f = storage.open(calibrationFile, "r");

    if (f != NULL)
    {
        if (repeatCalib)
            remove(calibrationFile);
        else
            if (fread((char *)calData, sizeof(char), 16, f))
            {
                calDataOK = 1;
                storage.close(f);
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

        TFT_eSprite touchSprite = TFT_eSprite(&tft);  
        touchSprite.createSprite(tft.width(), tft.height());  

        touchSprite.drawCenterString("TOUCH THE ARROW MARKER.", tft.width() >> 1, tft.height() >> 1, fontSmall);
        touchSprite.pushSprite(0,0);

        tft.calibrateTouch(calData, TFT_WHITE, TFT_BLACK, std::max(tft.width(), tft.height()) >> 3);
        touchSprite.drawCenterString("DONE!", tft.width() >> 1, (tft.height() >> 1) + (tft.fontHeight(fontSmall) * 2), fontLarge);
        touchSprite.pushSprite(0,0);
        delay(500);
        touchSprite.drawCenterString("TOUCH TO CONTINUE.", tft.width() >> 1, (tft.height() >> 1) + (tft.fontHeight(fontLarge) * 2), fontSmall);
        touchSprite.pushSprite(0,0);

        FILE* f = storage.open(calibrationFile, "w");
        if (f)
        {
            log_v("Calibration saved");
            fwrite((const unsigned char *)calData, sizeof(unsigned char), 16 ,f);
            storage.close(f);
        }
        else
            log_e("Calibration not saved!");

        uint16_t touchX, touchY;
        while (!tft.getTouch(&touchX, &touchY));

        touchSprite.deleteSprite();
    }
}

/**
 * @brief Init TFT display
 *
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

    #ifdef TOUCH_INPUT
        touchCalibrate();
    #endif
}
