/**
 * @file sensorScr.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  LVGL - Sensor Info Screen
 * @version 0.2.9
 * @date 2026-06
 */

#include "sensorScr.hpp"
#include "globalGuiDef.h"
#include <cmath>

lv_obj_t *sensorScreen = NULL;

static lv_timer_t *sensorUpdateTimer = NULL;

#ifdef BATT_PIN
static lv_obj_t *lblBattPct  = NULL;
static lv_obj_t *lblBattVolt = NULL;
static lv_obj_t *battBar     = NULL;
#endif

#ifdef BME280
static lv_obj_t *lblBmeTemp     = NULL;
static lv_obj_t *lblBmeHumi     = NULL;
static lv_obj_t *lblBmePres     = NULL;
static lv_obj_t *lblBmeAlt      = NULL;
#endif

#ifdef ENABLE_COMPASS
static lv_obj_t *lblHeading     = NULL;
static lv_obj_t *barHeading     = NULL;
#endif

#ifdef ENABLE_IMU
static lv_obj_t *barAccelX      = NULL;
static lv_obj_t *barAccelY      = NULL;
static lv_obj_t *barAccelZ      = NULL;
static lv_obj_t *lblAccelX      = NULL;
static lv_obj_t *lblAccelY      = NULL;
static lv_obj_t *lblAccelZ      = NULL;
static lv_obj_t *barGyroX       = NULL;
static lv_obj_t *barGyroY       = NULL;
static lv_obj_t *barGyroZ       = NULL;
static lv_obj_t *lblGyroX       = NULL;
static lv_obj_t *lblGyroY       = NULL;
static lv_obj_t *lblGyroZ       = NULL;
#endif

/**
 * @brief Creates a section card with a title label.
 *
 * @param parent Parent container.
 * @param title  Card title string.
 * @return Content container object.
 */
static lv_obj_t *createCard(lv_obj_t *parent, const char *title)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_width(card, TFT_WIDTH - 20);
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(card, 4, 0);

    lv_obj_t *lbl = lv_label_create(card);
    lv_obj_set_style_text_font(lbl, fontMedium, 0);
    lv_label_set_text_static(lbl, title);

    lv_obj_t *sep = lv_obj_create(card);
    lv_obj_set_size(sep, lv_pct(100), 1);
    lv_obj_set_style_bg_color(sep, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_30, 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    return card;
}

/**
 * @brief Creates a key/value row, returns the value label.
 *
 * @param parent Parent container.
 * @param key    Key label string.
 * @return Value label object.
 */
static lv_obj_t *createRow(lv_obj_t *parent, const char *key)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *keyLbl = lv_label_create(row);
    lv_obj_set_style_text_font(keyLbl, fontDefault, 0);
    lv_label_set_text_static(keyLbl, key);

    lv_obj_t *valLbl = lv_label_create(row);
    lv_obj_set_style_text_font(valLbl, fontDefault, 0);
    lv_label_set_text_static(valLbl, "--");

    return valLbl;
}

/**
 * @brief Creates a labelled bar row for IMU axes.
 *
 * @param parent   Parent container.
 * @param key      Axis label string.
 * @param minVal   Bar minimum value.
 * @param maxVal   Bar maximum value.
 * @param outLabel Receives the value label object.
 * @return Bar object.
 */
#ifdef ENABLE_IMU
static lv_obj_t *createBarRow(lv_obj_t *parent, const char *key, int32_t minVal, int32_t maxVal, lv_obj_t **outLabel)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 6, 0);

    lv_obj_t *keyLbl = lv_label_create(row);
    lv_obj_set_style_text_font(keyLbl, fontSmall, 0);
    lv_obj_set_width(keyLbl, (int)(22 * scale));
    lv_label_set_text_static(keyLbl, key);

    lv_obj_t *bar = lv_bar_create(row);
    lv_obj_set_height(bar, (int)(10 * scale));
    lv_obj_set_flex_grow(bar, 1);
    lv_bar_set_range(bar, minVal, maxVal);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);

    lv_obj_t *valLbl = lv_label_create(row);
    lv_obj_set_style_text_font(valLbl, fontSmall, 0);
    lv_obj_set_width(valLbl, (int)(50 * scale));
    lv_label_set_text_static(valLbl, "  0.00");

    *outLabel = valLbl;
    return bar;
}
#endif

/**
 * @brief Timer callback — reads globalSensorData and refreshes labels.
 */
static void sensorUpdateCb(lv_timer_t *timer)
{
    SensorData snap;

    if (sensorMutex != NULL && xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        snap = globalSensorData;
        xSemaphoreGive(sensorMutex);
    }
    else
        return;

    #ifdef BATT_PIN
    {
        static char voltBuf[12];
        snprintf(voltBuf, sizeof(voltBuf), "%.2f V", snap.batteryVoltage);
        lv_label_set_text(lblBattVolt, voltBuf);

        if (snap.batteryPercent > 110.0f)
        {
            lv_label_set_text_static(lblBattPct, LV_SYMBOL_CHARGE);
            lv_bar_set_value(battBar, 100, LV_ANIM_OFF);
            lv_obj_set_style_bg_color(battBar, lv_color_hex(0x1B5E20), LV_PART_INDICATOR);
        }
        else
        {
            static char pctBuf[12];
            snprintf(pctBuf, sizeof(pctBuf), "%.0f %%", snap.batteryPercent);
            lv_label_set_text(lblBattPct, pctBuf);
            int32_t pct = (int32_t)snap.batteryPercent;
            if (pct < 0)   pct = 0;
            if (pct > 100) pct = 100;
            lv_bar_set_value(battBar, pct, LV_ANIM_OFF);

            lv_color_t barColor;
            if (pct < 20)
                barColor = lv_color_hex(0xD32F2F);
            else if (pct < 40)
                barColor = lv_color_hex(0xFF6F00);
            else if (pct < 60)
                barColor = lv_color_hex(0xF9A825);
            else if (pct < 80)
                barColor = lv_color_hex(0x558B2F);
            else
                barColor = lv_color_hex(0x1B5E20);
            lv_obj_set_style_bg_color(battBar, barColor, LV_PART_INDICATOR);
        }
    }
    #endif

    #ifdef BME280
    {
        static char tBuf[16];
        static char hBuf[16];
        static char pBuf[16];
        static char aBuf[16];
        snprintf(tBuf, sizeof(tBuf), "%.1f °C",  snap.temperature);
        snprintf(hBuf, sizeof(hBuf), "%.1f %%",  snap.humidity);
        snprintf(pBuf, sizeof(pBuf), "%.0f hPa", snap.pressure / 100.0f);
        snprintf(aBuf, sizeof(aBuf), "%d m",      (int)snap.altitude);
        lv_label_set_text(lblBmeTemp, tBuf);
        lv_label_set_text(lblBmeHumi, hBuf);
        lv_label_set_text(lblBmePres, pBuf);
        lv_label_set_text(lblBmeAlt,  aBuf);
    }
    #endif

    #ifdef ENABLE_COMPASS
    {
        static char hBuf[8];
        snprintf(hBuf, sizeof(hBuf), "%d°", snap.heading);
        lv_label_set_text(lblHeading, hBuf);
        lv_bar_set_value(barHeading, snap.heading, LV_ANIM_OFF);
    }
    #endif

    #ifdef ENABLE_IMU
    {
        static char buf[16];

        auto accelColor = [](float absVal) -> lv_color_t
        {
            if (absVal < 0.5f)
                return lv_color_hex(0x1B5E20);
            else if (absVal < 1.0f)
                return lv_color_hex(0xFF6F00);
            else
                return lv_color_hex(0xD32F2F);
        };

        auto gyroColor = [](float absVal) -> lv_color_t
        {
            if (absVal < 50.0f)
                return lv_color_hex(0x1B5E20);
            else if (absVal < 100.0f)
                return lv_color_hex(0xFF6F00);
            else
                return lv_color_hex(0xD32F2F);
        };

        snprintf(buf, sizeof(buf), "%+.2f g", snap.accelX);
        lv_label_set_text(lblAccelX, buf);
        lv_bar_set_value(barAccelX, (int32_t)(snap.accelX * 100.0f), LV_ANIM_OFF);
        lv_obj_set_style_bg_color(barAccelX, accelColor(fabsf(snap.accelX)), LV_PART_INDICATOR);

        snprintf(buf, sizeof(buf), "%+.2f g", snap.accelY);
        lv_label_set_text(lblAccelY, buf);
        lv_bar_set_value(barAccelY, (int32_t)(snap.accelY * 100.0f), LV_ANIM_OFF);
        lv_obj_set_style_bg_color(barAccelY, accelColor(fabsf(snap.accelY)), LV_PART_INDICATOR);

        snprintf(buf, sizeof(buf), "%+.2f g", snap.accelZ);
        lv_label_set_text(lblAccelZ, buf);
        lv_bar_set_value(barAccelZ, (int32_t)(snap.accelZ * 100.0f), LV_ANIM_OFF);
        lv_obj_set_style_bg_color(barAccelZ, accelColor(fabsf(snap.accelZ)), LV_PART_INDICATOR);

        snprintf(buf, sizeof(buf), "%+6.1f", snap.gyroX);
        lv_label_set_text(lblGyroX, buf);
        lv_bar_set_value(barGyroX, (int32_t)(snap.gyroX), LV_ANIM_OFF);
        lv_obj_set_style_bg_color(barGyroX, gyroColor(fabsf(snap.gyroX)), LV_PART_INDICATOR);

        snprintf(buf, sizeof(buf), "%+6.1f", snap.gyroY);
        lv_label_set_text(lblGyroY, buf);
        lv_bar_set_value(barGyroY, (int32_t)(snap.gyroY), LV_ANIM_OFF);
        lv_obj_set_style_bg_color(barGyroY, gyroColor(fabsf(snap.gyroY)), LV_PART_INDICATOR);

        snprintf(buf, sizeof(buf), "%+6.1f", snap.gyroZ);
        lv_label_set_text(lblGyroZ, buf);
        lv_bar_set_value(barGyroZ, (int32_t)(snap.gyroZ), LV_ANIM_OFF);
        lv_obj_set_style_bg_color(barGyroZ, gyroColor(fabsf(snap.gyroZ)), LV_PART_INDICATOR);
    }
    #endif
}

/**
 * @brief Back button event — returns to settings screen.
 */
static void sensorBack(lv_event_t *event)
{
    if (sensorUpdateTimer != NULL)
    {
        lv_timer_delete(sensorUpdateTimer);
        sensorUpdateTimer = NULL;
    }
    lv_screen_load(settingsScreen);
}

/**
 * @brief Creates the Sensor Info screen.
 */
void createSensorScr()
{
    sensorScreen = lv_obj_create(NULL);

    lv_obj_t *scroll = lv_obj_create(sensorScreen);
    lv_obj_set_size(scroll, TFT_WIDTH, TFT_HEIGHT);
    lv_obj_set_flex_flow(scroll, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scroll, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(scroll, 10, 0);
    lv_obj_set_style_pad_all(scroll, 10, 0);
    lv_obj_add_style(scroll, &styleTransparent, LV_PART_MAIN);
    lv_obj_set_scroll_dir(scroll, LV_DIR_VER);
    lv_obj_add_flag(scroll, LV_OBJ_FLAG_SCROLLABLE);

    #ifdef BATT_PIN
    {
        lv_obj_t *card = createCard(scroll, "Battery");

        lv_obj_t *barRow = lv_obj_create(card);
        lv_obj_set_size(barRow, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(barRow, 0, 0);
        lv_obj_set_style_border_width(barRow, 0, 0);
        lv_obj_set_style_bg_opa(barRow, LV_OPA_TRANSP, 0);
        lv_obj_set_flex_flow(barRow, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(barRow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        battBar = lv_bar_create(barRow);
        lv_obj_set_height(battBar, (int)(14 * scale));
        lv_obj_set_flex_grow(battBar, 1);
        lv_bar_set_range(battBar, 0, 100);
        lv_bar_set_value(battBar, 0, LV_ANIM_OFF);

        lblBattPct = lv_label_create(barRow);
        lv_obj_set_style_text_font(lblBattPct, fontDefault, 0);
        lv_obj_set_width(lblBattPct, (int)(52 * scale));
        lv_label_set_text_static(lblBattPct, "-- %");

        lblBattVolt = createRow(card, "Voltage");
    }
    #endif

    #ifdef BME280
    {
        lv_obj_t *card = createCard(scroll, "Environment");
        lblBmeTemp = createRow(card, "Temp");
        lblBmeHumi = createRow(card, "Humidity");
        lblBmePres = createRow(card, "Pressure");
        lblBmeAlt  = createRow(card, "Altitude");
    }
    #endif

    #ifdef ENABLE_COMPASS
    {
        lv_obj_t *card = createCard(scroll, "Compass");

        lv_obj_t *barRow = lv_obj_create(card);
        lv_obj_set_size(barRow, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(barRow, 0, 0);
        lv_obj_set_style_border_width(barRow, 0, 0);
        lv_obj_set_style_bg_opa(barRow, LV_OPA_TRANSP, 0);
        lv_obj_set_flex_flow(barRow, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(barRow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        barHeading = lv_bar_create(barRow);
        lv_obj_set_height(barHeading, (int)(14 * scale));
        lv_obj_set_flex_grow(barHeading, 1);
        lv_bar_set_range(barHeading, 0, 359);
        lv_bar_set_value(barHeading, 0, LV_ANIM_OFF);

        lblHeading = lv_label_create(barRow);
        lv_obj_set_style_text_font(lblHeading, fontDefault, 0);
        lv_obj_set_width(lblHeading, (int)(48 * scale));
        lv_label_set_text_static(lblHeading, "---°");
    }
    #endif

    #ifdef ENABLE_IMU
    {
        lv_obj_t *card = createCard(scroll, "Accelerometer (g)");
        barAccelX = createBarRow(card, "X", -200, 200, &lblAccelX);
        barAccelY = createBarRow(card, "Y", -200, 200, &lblAccelY);
        barAccelZ = createBarRow(card, "Z", -200, 200, &lblAccelZ);

        lv_obj_t *card2 = createCard(scroll, "Gyroscope (°/s)");
        barGyroX = createBarRow(card2, "X", -250, 250, &lblGyroX);
        barGyroY = createBarRow(card2, "Y", -250, 250, &lblGyroY);
        barGyroZ = createBarRow(card2, "Z", -250, 250, &lblGyroZ);
    }
    #endif

    lv_obj_t *btn = lv_btn_create(scroll);
    lv_obj_set_size(btn, TFT_WIDTH - 30, (int)(40 * scale));
    lv_obj_t *btnLabel = lv_label_create(btn);
    lv_obj_set_style_text_font(btnLabel, fontLarge, 0);
    lv_label_set_text_static(btnLabel, "Back");
    lv_obj_center(btnLabel);
    lv_obj_add_event_cb(btn, sensorBack, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(sensorScreen, [](lv_event_t *e)
    {
        lv_obj_scroll_to_y(lv_obj_get_child(sensorScreen, 0), 0, LV_ANIM_OFF);
        if (sensorUpdateTimer == NULL)
            sensorUpdateTimer = lv_timer_create(sensorUpdateCb, 100, NULL);
    }, LV_EVENT_SCREEN_LOAD_START, NULL);

    lv_obj_add_event_cb(sensorScreen, [](lv_event_t *e)
    {
        if (sensorUpdateTimer != NULL)
        {
            lv_timer_delete(sensorUpdateTimer);
            sensorUpdateTimer = NULL;
        }
    }, LV_EVENT_SCREEN_UNLOADED, NULL);
}
