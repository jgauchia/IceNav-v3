/**
 * @file sattrack.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Satellite Tracking events
 * @version 0.1.5
 * @date 2023-06-04
 */

/**
 * @brief GNSS Selection Checkbox event
 *
 * @param event
 */
static void active_gnss_event(lv_event_t *event)
{
    uint32_t *active_id = (uint32_t *)lv_event_get_user_data(event);
    lv_obj_t *cont = lv_event_get_current_target(event);
    lv_obj_t *act_cb = lv_event_get_target(event);
    lv_obj_t *old_cb = lv_obj_get_child(cont, *active_id);

    if (act_cb == cont)
        return;

    lv_obj_clear_state(old_cb, LV_STATE_CHECKED);
    lv_obj_add_state(act_cb, LV_STATE_CHECKED);

    clear_sat_in_view();

    *active_id = lv_obj_get_index(act_cb);
}

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
#ifdef MULTI_GNSS
    switch ((int)active_gnss)
    {
    case 0:
        fill_sat_in_view(GPS_GSV, TFT_GREEN);
        break;
    case 1:
        fill_sat_in_view(GL_GSV, TFT_BLUE);
        break;
    case 2:
        fill_sat_in_view(BD_GSV, TFT_RED);
        break;
    }
#else
    fill_sat_in_view(GPS_GSV, TFT_GREEN);
#endif
}