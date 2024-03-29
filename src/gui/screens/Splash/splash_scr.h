/**
 * @file splash_scr.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Splash screen - NOT LVGL
 * @version 0.1.6
 * @date 2023-06-14
 */

/**
 * @brief Splash screen
 *
 */
void splash_scr()
{
    tft.fillScreen(TFT_BLACK);
    millis_actual = millis();
    set_brightness(0);
    tft.drawPngFile(SPIFFS, PSTR("/BOOTLOGO.png"), (tft.width() / 2) - 150 , (tft.height() / 2) - 70);
    char status_str[200] = "";
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    memset(&status_str[0],0,sizeof(status_str)); 
    sprintf(status_str, "Model:%s %dMhz - Free mem:%dK %d%%",ESP.getChipModel(),ESP.getCpuFreqMHz(),(ESP.getFreeHeap()/1024),(ESP.getFreeHeap()*100)/ESP.getHeapSize() );
    tft.drawString(status_str, 10, 450);
    memset(&status_str[0],0,sizeof(status_str)); 
    sprintf(status_str,"PSRAM: %d - Used PSRAM: %d", ESP.getPsramSize(), ESP.getPsramSize() - ESP.getFreePsram());
    tft.drawString(status_str, 10, 460);
    memset(&status_str[0],0,sizeof(status_str)); 
    sprintf(status_str, "Firmware v.%s rev.%s - %s",String(VERSION),String(REVISION), String(FLAVOR));
    tft.drawString(status_str, 10, 470);
    memset(&status_str[0],0,sizeof(status_str)); 
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    
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