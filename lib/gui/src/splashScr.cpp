/**
 * @file splashScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Splash screen - NOT LVGL
 * @version 0.3.0
 * @date 2026-06
 */

#include "splashScr.hpp"
#include "lv_subjects.hpp"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "soc/rtc.h"
#include "gpsMath.hpp"
#include "display.hpp"

lv_obj_t *splashCanvas;

extern Storage storage;

/**
 * @brief Get PNG width and height from file header.
 *
 * @param filename PNG file path on SD/SPIFFS.
 * @param width Pointer to store the image width in pixels.
 * @param height Pointer to store the image height in pixels.
 * @return true if the file exists and dimensions were read successfully, false otherwise.
 */
bool getPngSize(const char* filename, uint16_t *width, uint16_t *height)
{
    FILE* file = storage.open(filename, "r");
    if (!file)
        return false;

    uint8_t table[32];
    storage.read(file, table, 32);
    *width  = table[16] * 256 * 256 * 256 + table[17] * 256 * 256 + table[18] * 256 + table[19];
    *height = table[20] * 256 * 256 * 256 + table[21] * 256 * 256 + table[22] * 256 + table[23];
    storage.close(file);
    return true;
}

/**
 * @brief Get the ESP Chip Model 
 * 
 * @return const char* 
 */
static const char* getChipModel()
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    switch (chip_info.model)
    {
        case CHIP_ESP32:   
            return "ESP32";
        case CHIP_ESP32S2: 
            return "ESP32-S2";
        case CHIP_ESP32S3: 
            return "ESP32-S3";
        case CHIP_ESP32C3: 
            return "ESP32-C3";
        case CHIP_ESP32H2: 
            return "ESP32-H2";
        default:           
            return "Unknown";
    }
}

static unsigned long millisActual = 0; /**< Current value of the system timer in milliseconds */
extern Maps mapView;
extern Gps gps;

/**
 * @brief Create LVGL Splash Screen
 *
 * @details Creates the LVGL splash screen object and canvas.
 */
void createLVGLSplashScreen()
{
    splashScr  = lv_obj_create(NULL);  
    splashCanvas = lv_canvas_create(splashScr);

    lv_obj_t *osmInfo = lv_obj_create(splashScr);
    lv_obj_set_width(osmInfo, display().width());
    lv_obj_set_height(osmInfo,50 * scale);
    lv_obj_clear_flag(osmInfo, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(osmInfo, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(osmInfo, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(osmInfo, 0, 0);
    lv_obj_set_style_border_opa(osmInfo, 0, 0);
    lv_obj_t *label;
    label = lv_label_create(osmInfo);
    lv_obj_set_style_text_font(label, fontSplash, 0);
    lv_label_set_text(label, "Map data from OpenStreetMap - (c)OpenStreetMap");
    label = lv_label_create(osmInfo);
    lv_obj_set_style_text_font(label, fontSplash, 0);
    lv_label_set_text(label, "(c)OpenStreetMap contributors");
    lv_obj_set_align(osmInfo, LV_ALIGN_BOTTOM_MID);
    label = lv_label_create(splashScr);
    lv_obj_set_style_text_font(label, fontSplashVersion, 0);
    lv_label_set_text_fmt(label,statusLine4, String(VERSION).c_str(), String(REVISION).c_str());
    lv_obj_set_align(label, LV_ALIGN_CENTER);
    lv_obj_set_y(label, -130 * scale);
}

/**
 * @brief Splash screen
 *
 * @details Displays the splash screen with logo, device information, Preloads the map and initializes display settings.
 */
void splashScreen()
{
    setTime = false;

    #ifdef SPLASH_FULLSCREEN
        millisActual = millis_idf();

        display().setBrightness(defBright);

        TFT_eSprite splashSprite = TFT_eSprite(&tft);
        void *splashBuffer = splashSprite.createSprite(display().width(), display().height());
        splashSprite.drawPngFile(logoFile, 0, 0);
        lv_canvas_set_buffer(splashCanvas, splashBuffer, display().width(), display().height(), LV_COLOR_FORMAT_RGB565_SWAPPED);
        splashSprite.deleteSprite();

        lv_screen_load_anim(splashScr, LV_SCR_LOAD_ANIM_FADE_OUT, 2500, 0, false);
        for (int i = 0; i < 1000; i++)
        {
            lv_task_handler();  
            vTaskDelay(5);
        }     

        lv_obj_fade_out(splashScr, 2500,0);
        for (int i = 0; i < 300; i++)
        {
            lv_task_handler();  
            vTaskDelay(5);
        }     

        // Preload Map at the very end when all animations are gone
        mapView.currentMapTile = mapView.getMapTile(gps.gpsData.longitude, gps.gpsData.latitude, zoom, 0, 0);
        mapView.generateMap(zoom);

        if (lvgl_mutex != NULL && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            lv_obj_delete(splashScr);
            xSemaphoreGive(lvgl_mutex);
        }
    #else
        tftOff();
        
        TFT_eSprite splashSprite = TFT_eSprite(&tft);
        splashSprite.createSprite(display().width(), display().height());

        display().clear(0x0000);
        millisActual = millis_idf();
        display().setBrightness(0);

        static uint16_t pngHeight = 0;
        static uint16_t pngWidth = 0;
        static uint8_t margin = 0;

        getPngSize(logoFile, &pngWidth, &pngHeight);
        splashSprite.fillScreen(TFT_BLACK);
        splashSprite.drawPngFile(logoFile, (display().width() / 2) - (pngWidth / 2), (display().height() / 2) - pngHeight);

        #ifdef T4_S3
            splashSprite.setTextSize(2);
            margin = 2;
        #else
            splashSprite.setTextSize(1);
            margin = 1;
        #endif
        splashSprite.setTextColor(TFT_WHITE, TFT_BLACK);

        splashSprite.drawCenterString("Map data from OpenStreetMap.", display().width() >> 1, display().height() - 120*margin);
        splashSprite.drawCenterString("(c) OpenStreetMap", display().width() >> 1, display().height() - 110*margin);
        splashSprite.drawCenterString("(c) OpenStreetMap contributors", display().width() >> 1, display().height() - 100*margin);

        char statusString[50] = "";
        splashSprite.setTextColor(TFT_YELLOW, TFT_BLACK);

        memset(&statusString[0], 0, sizeof(statusString));
        rtc_cpu_freq_config_t freq_config;
        rtc_clk_cpu_freq_get_config(&freq_config);
        sprintf(statusString, statusLine1, getChipModel(), (int)freq_config.freq_mhz);
        splashSprite.drawString(statusString, 0, display().height() - 50*margin);

        memset(&statusString[0], 0, sizeof(statusString));
        size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        size_t totalHeap = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
        sprintf(statusString, statusLine2, (freeHeap / 1024), (freeHeap * 100) / totalHeap);
        splashSprite.drawString(statusString, 0, display().height() - 40*margin);

        memset(&statusString[0], 0, sizeof(statusString));
        sprintf(statusString, statusLine3, heap_caps_get_total_size(MALLOC_CAP_SPIRAM), heap_caps_get_total_size(MALLOC_CAP_SPIRAM) - heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        splashSprite.drawString(statusString, 0, display().height() - 30*margin);

        memset(&statusString[0], 0, sizeof(statusString));
        sprintf(statusString, statusLine4, String(VERSION), String(REVISION));
        splashSprite.drawString(statusString, 0, display().height() - 20*margin);

        memset(&statusString[0], 0, sizeof(statusString));
        sprintf(statusString, statusLine5, String(FLAVOR));
        splashSprite.drawString(statusString, 0, display().height() - 10*margin);

        memset(&statusString[0], 0, sizeof(statusString));
        splashSprite.setTextColor(TFT_WHITE, TFT_BLACK);
        
        const uint8_t maxBrightness = 255;

        tftOn(0);

        for (uint8_t fadeIn = 0; fadeIn <= (maxBrightness - 1); fadeIn++)
        {
            display().setBrightness(fadeIn);
            if (fadeIn == 0)
                splashSprite.pushSprite(0,0);
            millisActual = millis_idf();
            while (millis_idf() < millisActual + 15);
        }

        // Preload Map while logo is fully visible
        mapView.currentMapTile = mapView.getMapTile(gps.gpsData.longitude, gps.gpsData.latitude, zoom, 0, 0);
        mapView.generateMap(zoom);

        while (millis_idf() < millisActual + 100);

        for (uint8_t fadeOut = maxBrightness; fadeOut > 0; fadeOut--)
        {
            display().setBrightness(fadeOut);
            millisActual = millis_idf();
            while (millis_idf() < millisActual + 15);
        }

        display().clear(0x0000);

        while (millis_idf() < millisActual + 100);

        display().setBrightness(defBright);
    
        splashSprite.deleteSprite();
    #endif
}

