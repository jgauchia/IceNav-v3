/**
 * @file sattrack.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Satellite Tracking events
 * @version 0.1.4
 * @date 2023-05-23
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
 * @brief Update Satellite Tracking
 *
 * @param event
 */
static void update_sattrack(lv_event_t *event)
{
    if (pdop.isUpdated() || hdop.isUpdated() || vdop.isUpdated())
    {
        lv_label_set_text_fmt(pdop_label, "PDOP:\n%s", pdop.value());
        lv_label_set_text_fmt(hdop_label, "HDOP:\n%s", hdop.value());
        lv_label_set_text_fmt(vdop_label, "VDOP:\n%s", vdop.value());
    }

    if (GPS.altitude.isUpdated())
        lv_label_set_text_fmt(alt_label, "ALT:\n%4dm.", (int)GPS.altitude.meters());

    create_sat_spr(spr_Sat);
    create_const_spr(constel_spr);
    create_const_spr(constel_spr_bkg);

    /*********************************************************************************************/
    if (totalGPSMsg.isUpdated())
    {
        lv_chart_refresh(satbar_1);
        lv_chart_refresh(satbar_2);

        for (int i = 0; i < 4; ++i)
        {
            int no = atoi(satGPSNum[i].value());
            if (no >= 1 && no <= MAX_SATELLITES)
            {
                sat_tracker[no - 1].sat_num = atoi(satGPSNum[i].value());
                sat_tracker[no - 1].elev = atoi(elevGPS[i].value());
                sat_tracker[no - 1].azim = atoi(azimGPS[i].value());
                sat_tracker[no - 1].snr = atoi(snrGPS[i].value());
                sat_tracker[no - 1].active = true;
            }
        }

        uint8_t totalMessages = atoi(totalGPSMsg.value());
        uint8_t currentMessage = atoi(msgGPSNum.value());
        if (totalMessages == currentMessage)
        {
            create_snr_spr(spr_SNR1);
            create_snr_spr(spr_SNR2);

            for (int i = 0; i < (MAX_SATELLLITES_IN_VIEW / 2); i++)
            {
                satbar_ser1->y_points[i] = LV_CHART_POINT_NONE;
                satbar_ser2->y_points[i] = LV_CHART_POINT_NONE;
            }

            uint8_t active_sat = 0;
            for (int i = 0; i < MAX_SATELLITES; ++i)
            {
                if (sat_tracker[i].active && (sat_tracker[i].snr > 0))
                {
                    lv_point_t p;
                    lv_area_t a;
                    if (active_sat < (MAX_SATELLLITES_IN_VIEW / 2))
                    {
                        satbar_ser1->y_points[active_sat] = sat_tracker[i].snr;
                        lv_chart_get_point_pos_by_id(satbar_1, satbar_ser1, active_sat, &p);
                        spr_SNR1.setCursor(p.x - 2, 0);
                        spr_SNR1.print(sat_tracker[i].sat_num);
                    }
                    else
                    {
                        satbar_ser2->y_points[active_sat - (MAX_SATELLLITES_IN_VIEW / 2)] = sat_tracker[i].snr;
                        lv_chart_get_point_pos_by_id(satbar_2, satbar_ser2, (active_sat - (MAX_SATELLLITES_IN_VIEW / 2)), &p);
                        spr_SNR2.setCursor(p.x - 2, 0);
                        spr_SNR2.print(sat_tracker[i].sat_num);
                    }
                    active_sat++;

                    sat_pos = get_sat_pos(sat_tracker[i].elev, sat_tracker[i].azim);

                    spr_Sat.fillCircle(4, 4, 2, TFT_GREEN);
                    spr_Sat.pushSprite(&constel_spr, sat_pos.x, sat_pos.y, TFT_TRANSPARENT);
                    constel_spr.setCursor(sat_pos.x, sat_pos.y + 8);
                    constel_spr.print(i + 1);

                    if (sat_tracker[i].pos_x != sat_pos.x || sat_tracker[i].pos_y != sat_pos.y)
                    {
                        constel_spr_bkg.pushSprite(120, 30);
                    }

                    sat_tracker[i].pos_x = sat_pos.x;
                    sat_tracker[i].pos_y = sat_pos.y;
                }
            }
            constel_spr.pushSprite(120, 30);
        }

        lv_chart_refresh(satbar_1);
        spr_SNR1.pushSprite(0, 260);
        lv_chart_refresh(satbar_2);
        spr_SNR2.pushSprite(0, 345);
    }
}