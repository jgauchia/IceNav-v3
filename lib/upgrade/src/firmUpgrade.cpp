/**
 * @file firmUpgrade.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Firmware upgrade from SD functions
 * @version 0.2.1
 * @date 2025-05
 */

 #include "firmUpgrade.hpp"

 extern Storage storage;

 static const char* TAG PROGMEM = "Firmware Update";

/**
 * @brief Check if firmware file exist
 * 
 * @return true/false
 */
bool checkFileUpgrade()
{
  if (storage.exists(upgrdFile))
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
  ESP_LOGV(TAG, "Try to upgrade firmware...");
  FILE *firmware = storage.open(upgrdFile, "r");
  Update.onProgress(onUpgrdProcess);
  Update.begin(storage.size(upgrdFile), U_FLASH);
  FileStream firmwareStream(firmware);
  Update.writeStream(firmwareStream);
  if (Update.end())
    ESP_LOGV(TAG, "Upgrade finished!");
  else
  {
    ESP_LOGE(TAG, "Upgrade error!");
    lv_label_set_text_static(msgUprgdText, LV_SYMBOL_WARNING " Upgrade error!");
    lv_obj_clear_flag(btnMsgBack,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(contMeter,LV_OBJ_FLAG_HIDDEN);
  }
  storage.close(firmware);
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
  ESP_LOGV(TAG, "Firmware Upgrade process %d ...", (int)progress);
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
  ESP_LOGI(TAG, "Rebooting ESP32: ");
  ESP.restart();
}