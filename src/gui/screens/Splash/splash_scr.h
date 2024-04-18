/**
 * @file splash_scr.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Splash screen - NOT LVGL
 * @version 0.1.8
 * @date 2024-04
 */

/**
 * @brief Splash screen
 *
 */
void splashScreen()
{
    tft.fillScreen(TFT_BLACK);
    millisActual = millis();
    setBrightness(0);
    tft.drawPngFile(SPIFFS, PSTR("/BOOTLOGO.png"), (tft.width() / 2) - 150 , (tft.height() / 2) - 70);
    char statusString[200] = "";
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    memset(&statusString[0],0,sizeof(statusString)); 
    sprintf(statusString, "Model:%s %dMhz - Free mem:%dK %d%%",ESP.getChipModel(),ESP.getCpuFreqMHz(),(ESP.getFreeHeap()/1024),(ESP.getFreeHeap()*100)/ESP.getHeapSize() );
    tft.drawString(statusString, 10, 450);
    memset(&statusString[0],0,sizeof(statusString)); 
    sprintf(statusString,"PSRAM: %d - Used PSRAM: %d", ESP.getPsramSize(), ESP.getPsramSize() - ESP.getFreePsram());
    tft.drawString(statusString, 10, 460);
    memset(&statusString[0],0,sizeof(statusString)); 
    sprintf(statusString, "Firmware v.%s rev.%s - %s",String(VERSION),String(REVISION), String(FLAVOR));
    tft.drawString(statusString, 10, 470);
    memset(&statusString[0],0,sizeof(statusString)); 
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    
    delay(100);
    for (int fadeIn = 0; fadeIn <= 255; fadeIn++)
    {
        setBrightness(fadeIn);
        delay(17);
    }
    for (int fadeOut = 255; fadeOut >= 0; fadeOut--)
    {
        setBrightness(fadeOut);
        delay(17);
    }
    while (millis() < millisActual + 100)
        ;

    tft.fillScreen(TFT_BLACK);
    setBrightness(255);
}