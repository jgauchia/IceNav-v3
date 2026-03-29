/**
 * @file firmUpgrade.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Firmware upgrade from SD functions
 * @version 0.2.4
 * @date 2025-12
 */

 #include "firmUpgrade.hpp"
 #include <freertos/FreeRTOS.h>
 #include <freertos/task.h>
 #include "esp_system.h"

 extern Storage storage;

 static const char* TAG = "Firmware Update";

TFT_eSprite upgradeSprite = TFT_eSprite(&tft);  

/**
 * @brief Check if firmware file exists
 * 
 * @details Checks if the firmware upgrade file is present in storage.
 * 
 * @return true if the upgrade file exists, false otherwise
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
 * @details Initiates the firmware upgrade by opening the firmware file, starting the update process, 
 * 			and handling upgrade progress and result.
 */
void onUpgrdStart()
{
    upgradeSprite.createSprite(tft.width(),tft.height());  
    ESP_LOGV(TAG, "Try to upgrade firmware...");
    FILE *firmware = storage.open(upgrdFile, "r");
    if (firmware)
    {
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
        upgradeSprite.deleteSprite();
        storage.close(firmware);
    }
    else
        ESP_LOGE(TAG, "Upgrade firmware error , file open error");
}

/**
 * @brief Draw progress bar
 * 
 * @details Draws a rounded progress bar at the specified position, with given width, height, percentage, and colors.
 * 
 * @param x           X position of the progress bar
 * @param y           Y position of the progress bar
 * @param w           Bar width
 * @param h           Bar height
 * @param percent     Percentage to show (0-100)
 * @param frameColor  Frame color of the bar
 * @param barColor    Fill color of the bar
 */
void drawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t percent, uint16_t frameColor, uint16_t barColor) 
{
    if (percent == 0)
        upgradeSprite.fillRoundRect(x, y, w, h, 3, TFT_BLACK);
    uint8_t margin = 2;
    uint16_t barHeight = h - 2 * margin;
    uint16_t barWidth  = w - 2 * margin;
    upgradeSprite.drawRoundRect(x, y, w, h, 3, frameColor);
    upgradeSprite.fillRect(x + margin, y + margin, barWidth * percent / 100.0f, barHeight, barColor);
}

/**
 * @brief Firmware upgrade process callback
 * 
 * @details Handles the progress display during firmware upgrade by updating the progress bar and showing percentage.
 * 
 * @param currSize   Current written size from file
 * @param totalSize  Total size of the firmware file
 */
void onUpgrdProcess(size_t currSize, size_t totalSize)
{
    float progress = (currSize * 100) / totalSize;
    ESP_LOGV(TAG, "Firmware Upgrade process %d ...", (int)progress);
    char strProgress[30];
    sprintf(strProgress,"Upgrading... %d%%",(int)progress);
    upgradeSprite.drawCenterString(strProgress, tft.width() >> 1, (tft.height() >> 1)+25, &fonts::FreeSans9pt7b);
    drawProgressBar(40,tft.height() >> 1,TFT_WIDTH - 80,20,(int)progress,TFT_WHITE,TFT_BLUE);
    if ((int)progress == 100)
        upgradeSprite.drawCenterString("Upgrade complete", tft.width() >> 1, (tft.height() >> 1)+25, &fonts::FreeSans9pt7b);

    upgradeSprite.pushSprite(0,0);
}

/**
 * @brief Firmware upgrade end callback
 * 
 * @details Called when the firmware upgrade process finishes. 
 */
void onUpgrdEnd()
{
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_LOGI(TAG, "Rebooting ESP32: ");
    esp_restart();
}