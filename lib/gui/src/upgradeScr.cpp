/**
 * @file upgradeScr.cpp
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief  LVGL Firmware upgrade messages
 * @version 0.1.9
 * @date 2024-12
 */

 #include "upgradeScr.hpp"

 lv_obj_t *msgUpgrade;
 lv_obj_t *msgUprgdText;
 lv_obj_t *btnMsgBack;
 lv_obj_t *btnMsgUpgrade;
 lv_obj_t *contMeter;

/**
 * @brief Upgrade Back Message Event
 * 
 */
void msgBackEvent(lv_event_t *event)
{
    lv_screen_load(deviceSettingsScreen);
}

/**
 * @brief Upgrade Message Event
 * 
 */
void msgUpgrdEvent(lv_event_t *event)
{
    log_v("Upgrade firmware");
    lv_obj_add_flag(btnMsgBack,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btnMsgUpgrade, LV_OBJ_FLAG_HIDDEN);
    onUpgrdStart();
    onUpgrdEnd();
}


/**
 * @brief Create Upgrade Message
 * 
 */
void createMsgUpgrade()
{
    msgUpgrade = lv_obj_create(NULL);
    lv_obj_t *msgBox = lv_msgbox_create(msgUpgrade);
    lv_msgbox_add_title(msgBox, "Firmware Upgrade");
    lv_obj_t *content = lv_msgbox_get_content(msgBox);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_height(msgBox,200);

    lv_obj_t *contText = lv_obj_create(content);
    lv_obj_set_size(contText, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(contText, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(contText, LV_FLEX_ALIGN_CENTER,  LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(contText,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_color(contText,lv_color_black(),0);

    msgUprgdText = lv_label_create(contText);
    lv_label_set_text_static(msgUprgdText,"");
    lv_obj_set_style_text_font(msgUprgdText, fontLarge, 0);

    contMeter = lv_obj_create(content);
    lv_obj_set_size(contMeter, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(contMeter, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(contMeter, LV_FLEX_ALIGN_CENTER,  LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(contMeter,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_color(contMeter,lv_color_black(),0);
    lv_obj_add_flag(contMeter,LV_OBJ_FLAG_HIDDEN);

    static uint32_t objectColor = 0x303030; 
    static lv_style_t styleBtn;
    lv_style_init(&styleBtn);
    lv_style_set_bg_color(&styleBtn, lv_color_hex(objectColor));
    lv_style_set_border_color(&styleBtn, lv_color_hex(objectColor));

    btnMsgBack = lv_msgbox_add_footer_button(msgBox, "Back");
    lv_obj_add_event_cb(btnMsgBack, msgBackEvent, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_text_font(btnMsgBack, fontLarge, 0);
    lv_obj_add_style(btnMsgBack, &styleBtn, 0);

    btnMsgUpgrade = lv_msgbox_add_footer_button(msgBox, "UPGRADE");
    lv_obj_add_event_cb(btnMsgUpgrade, msgUpgrdEvent, LV_EVENT_CLICKED, NULL); 
    lv_obj_set_style_text_font(btnMsgUpgrade, fontLarge, 0);
    lv_obj_add_style(btnMsgUpgrade, &styleBtn, 0);
    lv_obj_add_flag(btnMsgUpgrade,LV_OBJ_FLAG_HIDDEN);
}