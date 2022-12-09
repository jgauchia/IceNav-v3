/**
 * @file splash_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Splash screen - NOT LVGL
 * @version 0.1
 * @date 2022-10-10
 */

/**
 * @brief Splash screen
 *
 */
void splash_scr()
{
    millis_actual = millis();
    set_brightness(0);
    tft.drawPngFile(SD, "/GFX/BOOTLOGO.png", (tft.width() / 2) - 85, (tft.height() / 2) - 70);
    delay(100);
    for (int fadein = 0; fadein <= 255; fadein++)
    {
        set_brightness(fadein);
        delay(17);
    }
    for (int fadeout = 255; fadeout >= 0; fadeout--)
    {
        set_brightness(fadeout);
        delay(17);
    }
    while (millis() < millis_actual + 100)
        ;

    tft.fillScreen(TFT_BLACK);
    set_brightness(255);
}