/**
 * @file sattrack.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Satellite Tracking events
 * @version 0.1.4
 * @date 2023-05-23
 */

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
    fill_sat_in_view(GPS_GSV,TFT_GREEN);
}