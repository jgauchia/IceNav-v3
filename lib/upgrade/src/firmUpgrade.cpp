/**
 * @file firmUpgrade.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Firmware upgrade from SD functions
 * @version 0.1.9
 * @date 2024-12
 */

 #include "firmUpgrade.hpp"

/**
 * @brief Check if firmware file exist
 * 
 * @return true/false
 */
bool checkFileUpgrade()
{
if (SD.exists(upgrdFile))
    return true;
else
    return false;
}

/**
 * @brief Firmware upgrade start callback
 * 
 */
void onUpgrdStart()
{
    log_v("Try to upgrade firmware...");
    File firmware = SD.open(upgrdFile);
    Update.onProgress(onUpgrdProcess);
    Update.begin(firmware.size(), U_FLASH);
    Update.writeStream(firmware);
    if (Update.end())
    {
        log_v("Upgrade finished!");
    }
    else
    {
        log_e("Upgrade error!");
        lv_label_set_text_static(msgUprgdText, LV_SYMBOL_WARNING " Upgrade error!");
        lv_obj_clear_flag(btnMsgBack,LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(contMeter,LV_OBJ_FLAG_HIDDEN);
    }
    firmware.close();
}

/**
 * @brief Draw progress bar
 * 
 * @param x -> X Position
 * @param y -> Y Position
 * @param w -> Bar width
 * @param h -> Bar height
 * @param percent -> Percent to show
 * @param frameColor -> Bar frame color
 * @param barColor -> Bar color
 */
void drawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t percent, uint16_t frameColor, uint16_t barColor) 
{
  if (percent == 0)
    tft.fillRoundRect(x, y, w, h, 3, TFT_BLACK);
  uint8_t margin = 2;
  uint16_t barHeight = h - 2 * margin;
  uint16_t barWidth  = w - 2 * margin;
  tft.drawRoundRect(x, y, w, h, 3, frameColor);
  tft.fillRect(x + margin, y + margin, barWidth * percent / 100.0, barHeight, barColor);
}

/**
 * @brief Firmware upgrade process callback
 * 
 * @param currSize -> Current size from file
 * @param totalSize -> Total size from file
 */
void onUpgrdProcess(size_t currSize, size_t totalSize)
{
    float progress = (currSize * 100) / totalSize;
    log_v("Firmware Upgrade process %d ...", (int)progress);
    char strProgress[30];
    sprintf(strProgress,"Upgrading... %d%%",(int)progress);
    tft.drawCenterString(strProgress, tft.width() >> 1, (tft.height() >> 1)+25, &fonts::FreeSans9pt7b);
    drawProgressBar(40,tft.height() >> 1,TFT_WIDTH - 80,20,(int)progress,TFT_WHITE,TFT_BLUE);
    if ((int)progress == 100)
        tft.drawCenterString("Upgrade complete", tft.width() >> 1, (tft.height() >> 1)+25, &fonts::FreeSans9pt7b);
}

/**
 * @brief Firmware upgrade end callback
 * 
 */
void onUpgrdEnd()
{
    delay(500);
    log_i("Rebooting ESP32: ");
    ESP.restart();
}