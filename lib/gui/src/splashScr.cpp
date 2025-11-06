/**
 * @file splashScr.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Splash screen - NOT LVGL
 * @version 0.2.3
 * @date 2025-11
 */

#include "splashScr.hpp"

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
	lv_obj_set_width(osmInfo, tft.width());
	lv_obj_set_height(osmInfo,50 * scale);
	lv_obj_clear_flag(osmInfo, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(osmInfo, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(osmInfo, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(osmInfo, 0, 0);
    lv_obj_set_style_border_opa(osmInfo, 0, 0);
	lv_obj_t *label;
	label = lv_label_create(osmInfo);
	lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
	lv_label_set_text(label, "Map data from OpenStreetMap - (c)OpenStreetMap");
	label = lv_label_create(osmInfo);
	lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
	lv_label_set_text(label, "(c)OpenStreetMap contributors");
	lv_obj_set_align(osmInfo, LV_ALIGN_BOTTOM_MID);
	label = lv_label_create(splashScr);
	lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
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

	// Preload Map
	mapView.currentMapTile = mapView.getMapTile(gps.gpsData.longitude, gps.gpsData.latitude, zoom, 0, 0);
	mapView.generateMap(zoom);

#ifdef ICENAV_BOARD
	millisActual = millis();

	tft.setBrightness(defBright);

	TFT_eSprite splashSprite = TFT_eSprite(&tft); 
	void *splashBuffer = splashSprite.createSprite(tft.width(), tft.height());
	splashSprite.drawPngFile(logoFile, 0, 0);
	lv_canvas_set_buffer(splashCanvas, splashBuffer, tft.width(), tft.height(), LV_COLOR_FORMAT_RGB565_SWAPPED);
	splashSprite.deleteSprite();

	lv_screen_load_anim(splashScr, LV_SCR_LOAD_ANIM_FADE_OUT, 2500, 0, false);	
	for( int i=0; i < 1000; i++ )
	{
		lv_task_handler();  
		vTaskDelay(5);
	}     
	lv_obj_fade_out(splashScr, 2500,0);
	for( int i=0; i < 300; i++ )
	{
		lv_task_handler();  
		vTaskDelay(5);
	}     
	lv_obj_delete(splashScr);
#else
	tft.fillScreen(TFT_BLACK);
	millisActual = millis();
	tft.setBrightness(0);

	static uint16_t pngHeight = 0;
	static uint16_t pngWidth = 0;

	getPngSize(logoFile, &pngWidth, &pngHeight);
	tft.drawPngFile(logoFile, (tft.width() / 2) - (pngWidth / 2), (tft.height() / 2) - pngHeight);

	tft.setTextSize(1);
	tft.setTextColor(TFT_WHITE, TFT_BLACK);

	tft.drawCenterString("Map data from OpenStreetMap.", tft.width() >> 1, TFT_HEIGHT - 120);
	tft.drawCenterString("(c) OpenStreetMap", tft.width() >> 1, TFT_HEIGHT - 110);
	tft.drawCenterString("(c) OpenStreetMap contributors", tft.width() >> 1, TFT_HEIGHT - 100);

	char statusString[50] = "";
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);

	memset(&statusString[0], 0, sizeof(statusString));
	sprintf(statusString, statusLine1, ESP.getChipModel(), ESP.getCpuFreqMHz());
	tft.drawString(statusString, 0, TFT_HEIGHT - 50);

	memset(&statusString[0], 0, sizeof(statusString));
	sprintf(statusString, statusLine2, (ESP.getFreeHeap() / 1024), (ESP.getFreeHeap() * 100) / ESP.getHeapSize());
	tft.drawString(statusString, 0, TFT_HEIGHT - 40);

	memset(&statusString[0], 0, sizeof(statusString));
	sprintf(statusString, statusLine3, ESP.getPsramSize(), ESP.getPsramSize() - ESP.getFreePsram());
	tft.drawString(statusString, 0, TFT_HEIGHT - 30);

	memset(&statusString[0], 0, sizeof(statusString));
	sprintf(statusString, statusLine4, String(VERSION), String(REVISION));
	tft.drawString(statusString, 0, TFT_HEIGHT - 20);

	memset(&statusString[0], 0, sizeof(statusString));
	sprintf(statusString, statusLine5, String(FLAVOR));
	tft.drawString(statusString, 0, TFT_HEIGHT - 10);

	memset(&statusString[0], 0, sizeof(statusString));
	tft.setTextColor(TFT_WHITE, TFT_BLACK);

	const uint8_t maxBrightness = 255;

	for (uint8_t fadeIn = 0; fadeIn <= (maxBrightness - 1); fadeIn++)
	{
		tft.setBrightness(fadeIn);
		millisActual = millis();
		while (millis() < millisActual + 15);
	}

	millisActual = millis();

	while (millis() < millisActual + 100);

	for (uint8_t fadeOut = maxBrightness; fadeOut > 0; fadeOut--)
	{
		tft.setBrightness(fadeOut);
		millisActual = millis();
		while (millis() < millisActual + 15);
	}

	tft.fillScreen(TFT_BLACK);

	while (millis() < millisActual + 100);

	tft.setBrightness(defBright);
#endif
}
