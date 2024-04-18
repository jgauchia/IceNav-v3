/**
 * @file sattrack.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  Satellite Tracking events
 * @version 0.1.8
 * @date 2024-04
 */

/**
 * @brief GNSS Selection Checkbox event
 *
 * @param event
 */
static void activeGnssEvent(lv_event_t *event)
{
    uint32_t *activeId = (uint32_t *)lv_event_get_user_data(event);
    lv_obj_t *cont = (lv_obj_t*)lv_event_get_current_target(event);
    lv_obj_t *activeCheckBox = (lv_obj_t*)lv_event_get_target(event);
    lv_obj_t *oldCheckBox = lv_obj_get_child(cont, *activeId);

    if (activeCheckBox == cont)
        return;

    lv_obj_clear_state(oldCheckBox, LV_STATE_CHECKED);
    lv_obj_add_state(activeCheckBox, LV_STATE_CHECKED);

    clearSatInView();

    *activeId = lv_obj_get_index(activeCheckBox);
}

/**
 * @brief Update Satellite Tracking
 *
 * @param event
 */
static void updateSatTrack(lv_event_t *event)
{
    if (pdop.isUpdated() || hdop.isUpdated() || vdop.isUpdated())
    {
        lv_label_set_text_fmt(pdopLabel, "PDOP:\n%s", pdop.value());
        lv_label_set_text_fmt(hdopLabel, "HDOP:\n%s", hdop.value());
        lv_label_set_text_fmt(vdopLabel, "VDOP:\n%s", vdop.value());
    }

    if (GPS.altitude.isUpdated())
        lv_label_set_text_fmt(altLabel, "ALT:\n%4dm.", (int)GPS.altitude.meters());

    createSatSprite(spriteSat);
    createConstelSprite(constelSprite);
    createConstelSprite(constelSpriteBkg);

#ifdef MULTI_GNSS
    switch ((int)activeGnss)
    {
    case 0:
        fillSatInView(GPS_GSV, TFT_GREEN);
        break;
    case 1:
        fillSatInView(GL_GSV, TFT_BLUE);
        break;
    case 2:
        fillSatInView(BD_GSV, TFT_RED);
        break;
    }
#else
    fillSatInView(GPS_GSV, TFT_GREEN);
#endif
}