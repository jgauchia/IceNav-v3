/**
 * @file sattrack.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Satellite Tracking events
 * @version 0.1
 * @date 2023-02-22
 */

/**
 * @brief Satellite Signal Graphics Bars Definitions
 *
 */
lv_obj_t *satbar_1;
lv_obj_t *satbar_2;
lv_chart_series_t *satbar_ser1;
lv_chart_series_t *satbar_ser2;

/**
 * @brief Sprite for snrGPS Satellite Labels
 *
 */
TFT_eSprite sprSNR1 = TFT_eSprite(&tft);
TFT_eSprite sprSNR2 = TFT_eSprite(&tft);

/**
 * @brief Sprite for satellite position in map
 *
 */
TFT_eSprite sprSat = TFT_eSprite(&tft);

/**
 * @brief Update Satellite Tracking
 *
 * @param event
 */
static void update_sattrack(lv_event_t *event)
{
    sprSat.deleteSprite();
    sprSat.createSprite(14, 14);
    sprSat.setColorDepth(16);
    sprSat.fillScreen(LVGL_BKG);

    if (pdop.isUpdated() || hdop.isUpdated() || vdop.isUpdated())
    {
        lv_label_set_text_fmt(pdop_label, "PDOP:\n%s", pdop.value());
        lv_label_set_text_fmt(hdop_label, "HDOP:\n%s", hdop.value());
        lv_label_set_text_fmt(vdop_label, "VDOP:\n%s", vdop.value());
    }

    if (GPS.altitude.isUpdated())
        lv_label_set_text_fmt(alt_label, "ALT:\n%4dm.", (int)GPS.altitude.meters());

    lv_chart_refresh(satbar_1);
    lv_chart_refresh(satbar_2);

    if (totalGPSMsg.isUpdated())
    {
        for (int i = 0; i < 4; ++i)
        {
            int no = atoi(satGPSNum[i].value());
            if (no >= 1 && no <= MAX_SATELLITES)
            {
                sat_tracker[no - 1].satGPSNum = atoi(satGPSNum[i].value());
                sat_tracker[no - 1].elevGPS = atoi(elevGPS[i].value());
                sat_tracker[no - 1].aziGPS = atoi(aziGPS[i].value());
                sat_tracker[no - 1].snrGPS = atoi(snrGPS[i].value());
                sat_tracker[no - 1].active = true;
            }
        }

        int totalMessages = atoi(totalGPSMsg.value());
        int currentMessage = atoi(msgGPSNum.value());
        if (totalMessages == currentMessage)
        {

            sprSNR1.deleteSprite();
            sprSNR1.createSprite(TFT_WIDTH, 10);
            sprSNR1.fillScreen(LVGL_BKG);
            sprSNR1.setTextColor(TFT_WHITE, LVGL_BKG);
            sprSNR2.deleteSprite();
            sprSNR2.createSprite(TFT_WIDTH, 10);
            sprSNR2.fillScreen(LVGL_BKG);
            sprSNR2.setTextColor(TFT_WHITE, LVGL_BKG);

            for (int i = 0; i < MAX_SATELLITES; ++i)
            {
                if (sat_tracker[i].pos_x != 0 && sat_tracker[i].pos_y != 0)
                {
                    sprSat.fillScreen(LVGL_BKG);
                    sprSat.pushSprite(sat_tracker[i].pos_x, sat_tracker[i].pos_y, TFT_TRANSPARENT);
                    tft.setCursor(sat_tracker[i].pos_x, sat_tracker[i].pos_y + 5);
                    tft.print("  ");
                }
            }

            tft.startWrite();
            tft.drawCircle(195, 100, 60, TFT_WHITE);
            tft.drawCircle(195, 100, 30, TFT_WHITE);
            tft.drawCircle(195, 100, 1, TFT_WHITE);
            tft.setTextFont(2);
            tft.setTextColor(TFT_WHITE, LVGL_BKG);
            tft.drawString("N", 192, 32);
            tft.drawString("S", 192, 152);
            tft.drawString("W", 132, 92);
            tft.drawString("E", 252, 92);
            tft.setTextFont(1);
            tft.endWrite();

            for (int i = 0; i < (MAX_SATELLLITES_IN_VIEW / 2); i++)
            {
                satbar_ser1->y_points[i] = LV_CHART_POINT_NONE;
                satbar_ser2->y_points[i] = LV_CHART_POINT_NONE;
            }

            int active_sat = 0;
            for (int i = 0; i < MAX_SATELLITES; ++i)
            {
                if (sat_tracker[i].active && (sat_tracker[i].snrGPS > 0))
                {
                    lv_point_t p;
                    lv_area_t a;
                    if (active_sat < (MAX_SATELLLITES_IN_VIEW / 2))
                    {
                        satbar_ser1->y_points[active_sat] = sat_tracker[i].snrGPS;
                        lv_chart_get_point_pos_by_id(satbar_1, satbar_ser1, active_sat, &p);
                        sprSNR1.setCursor(p.x - 2, 0);
                        sprSNR1.print(sat_tracker[i].satGPSNum);
                    }
                    else
                    {
                        satbar_ser2->y_points[active_sat - (MAX_SATELLLITES_IN_VIEW / 2)] = sat_tracker[i].snrGPS;
                        lv_chart_get_point_pos_by_id(satbar_2, satbar_ser2, (active_sat - (MAX_SATELLLITES_IN_VIEW / 2)), &p);
                        sprSNR2.setCursor(p.x - 2, 0);
                        sprSNR2.print(sat_tracker[i].satGPSNum);
                    }
                    active_sat++;

                    int H = (60 * cos(DEGtoRAD(sat_tracker[i].elevGPS)));
                    int sat_pos_x = 195 + (H * sin(DEGtoRAD(sat_tracker[i].aziGPS)));
                    int sat_pos_y = 100 - (H * cos(DEGtoRAD(sat_tracker[i].aziGPS)));

                    sprSat.fillCircle(7, 2, 2, TFT_GREEN);
                    sprSat.pushSprite(sat_pos_x, sat_pos_y, TFT_TRANSPARENT);
                    tft.setCursor(sat_pos_x, sat_pos_y + 5);
                    tft.print(i + 1);
                    sat_tracker[i].pos_x = sat_pos_x;
                    sat_tracker[i].pos_y = sat_pos_y;
                }
            }
        }
        lv_chart_refresh(satbar_1);
        sprSNR1.pushSprite(0, 260);
        lv_chart_refresh(satbar_2);
        sprSNR2.pushSprite(0, 345);
    }
}