#include "gui.h"
#include "lv_conf_internal.h"
#include "stdbool.h"
// #include "esp_wifi_types.h"

#if LV_USE_MENU && LV_USE_MSGBOX && LV_BUILD_EXAMPLES

enum {
    LV_MENU_ITEM_BUILDER_VARIANT_1,
    LV_MENU_ITEM_BUILDER_VARIANT_2
};
typedef uint8_t lv_menu_builder_variant_t;


extern lv_indev_t * indev_encoder;
extern lv_group_t * group;
// extern void wifi_start(void);
// extern void wifi_stop(void);
// extern wifi_ap_record_t *  wifi_scan(void);
extern void lcd_backlight_adjustment(uint16_t duty);
// extern bool wifi_scan_state;
// extern uint16_t ap_count;

static lv_obj_t * menu;
static lv_obj_t * cont;
static lv_obj_t * section;
static lv_obj_t * sub_wifi_page;

static void back_event_handler(lv_event_t * e);
static void switch_handler(lv_event_t * e);
static void BLK_Adjust_handler(lv_event_t * e);
lv_obj_t * root_page;
static lv_obj_t * create_text(lv_obj_t * parent, const char * icon, const char * txt,
                              lv_menu_builder_variant_t builder_variant);
static lv_obj_t * create_slider(lv_obj_t * parent,
                                const char * icon, const char * txt, int32_t min, int32_t max, int32_t val);
static lv_obj_t * create_switch(lv_obj_t * parent,
                                const char * icon, const char * txt, bool chk);

void setting_creat(void)
{
    group = lv_group_create();
	lv_group_set_default(group);
	lv_indev_set_group(indev_encoder, group);

    menu = lv_menu_create(lv_scr_act());

    lv_color_t bg_color = lv_obj_get_style_bg_color(menu, 0);
    if(lv_color_brightness(bg_color) > 127) {
        lv_obj_set_style_bg_color(menu, lv_color_darken(lv_obj_get_style_bg_color(menu, 0), 10), 0);
    }
    else {
        lv_obj_set_style_bg_color(menu, lv_color_darken(lv_obj_get_style_bg_color(menu, 0), 50), 0);
    }
    lv_menu_set_mode_root_back_btn(menu, LV_MENU_ROOT_BACK_BTN_ENABLED);
    lv_obj_add_event_cb(menu, back_event_handler, LV_EVENT_CLICKED, menu);
    lv_obj_set_size(menu, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_center(menu);



    /*Create sub pages*/
    lv_obj_t * sub_mechanics_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_mechanics_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_mechanics_page);
    section = lv_menu_section_create(sub_mechanics_page);
    create_slider(section, LV_SYMBOL_SETTINGS, "Velocity", 0, 100, 20);
    create_slider(section, LV_SYMBOL_SETTINGS, "Acceleration", 0, 100, 50);
    create_slider(section, LV_SYMBOL_SETTINGS, "Weight limit", 0, 100, 80);

    sub_wifi_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_wifi_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_wifi_page);
    section = lv_menu_section_create(sub_wifi_page);
    cont = create_switch(section, LV_SYMBOL_WIFI, "Wifi", false);
    lv_obj_add_event_cb(lv_obj_get_child(cont, 2), switch_handler, LV_EVENT_VALUE_CHANGED, menu);

    lv_obj_t * sub_display_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_display_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_display_page);
    section = lv_menu_section_create(sub_display_page);
    cont = create_slider(section, LV_SYMBOL_IMAGE, "Brightness", 0, 100, 0);
    lv_obj_add_event_cb(lv_obj_get_child(cont, 2), BLK_Adjust_handler, LV_EVENT_VALUE_CHANGED, menu);

    lv_obj_t * sub_software_info_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_software_info_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    section = lv_menu_section_create(sub_software_info_page);
    create_text(section, NULL, "\tVersion 1.0", LV_MENU_ITEM_BUILDER_VARIANT_1);

    lv_obj_t * sub_legal_info_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_legal_info_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    section = lv_menu_section_create(sub_legal_info_page);
    create_text(section, NULL, "LVGL v8.3.1", LV_MENU_ITEM_BUILDER_VARIANT_1);
    create_text(section, NULL, "\n\n\n\n\n\n\n\n\nCopyright (C) 2022 Square,Inc.", LV_MENU_ITEM_BUILDER_VARIANT_1);
 

    lv_obj_t * sub_about_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_about_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_about_page);
    section = lv_menu_section_create(sub_about_page);
    cont = create_text(section, NULL, "Software information", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_software_info_page);
    lv_group_add_obj(group,cont);
    cont = create_text(section, NULL, "Legal information", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_legal_info_page);
    lv_group_add_obj(group,cont);

    // lv_obj_t * sub_menu_mode_page = lv_menu_page_create(menu, NULL);
    // lv_obj_set_style_pad_hor(sub_menu_mode_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    // lv_menu_separator_create(sub_menu_mode_page);
    // section = lv_menu_section_create(sub_menu_mode_page);
    // cont = create_switch(section, LV_SYMBOL_AUDIO, "Sidebar enable", false);
    // lv_obj_add_event_cb(lv_obj_get_child(cont, 2), switch_handler, LV_EVENT_VALUE_CHANGED, menu);

    /*Create a root page*/
    root_page = lv_menu_page_create(menu, "Settings");
    lv_obj_set_style_pad_hor(root_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    section = lv_menu_section_create(root_page);
    cont = create_text(section, LV_SYMBOL_SETTINGS, "Mechanics", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_mechanics_page);
    lv_group_add_obj(group,cont);
    cont = create_text(section, LV_SYMBOL_WIFI, "Wifi", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_wifi_page);
    lv_group_add_obj(group,cont);
    cont = create_text(section, LV_SYMBOL_IMAGE, "Display", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_display_page);
    lv_group_add_obj(group,cont);

    create_text(root_page, NULL, "Others", LV_MENU_ITEM_BUILDER_VARIANT_1);
    section = lv_menu_section_create(root_page);
    cont = create_text(section, NULL, "About", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_about_page);
    lv_group_add_obj(group,cont);


    lv_menu_set_sidebar_page(menu, NULL);
    lv_menu_clear_history(menu); /* Clear history because we will be showing the root page later */
    lv_menu_set_page(menu, root_page);
}

static void back_event_handler(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_obj_t * menu = lv_event_get_user_data(e);

    if(lv_menu_back_btn_is_root(menu, obj)) {
        
    lv_obj_del(menu);
    desktop_creat();
    group = NULL;
    }
}

static void switch_handler(lv_event_t * e)
{
    // uint8_t i;
    lv_event_code_t code = lv_event_get_code(e);
    // lv_obj_t * menu = lv_event_get_user_data(e);
    // lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
    //     if(lv_obj_has_state(obj, LV_STATE_CHECKED)) {
    //         wifi_start();
    //         wifi_ap_record_t * ap_info = wifi_scan();
    // section = lv_menu_section_create(sub_wifi_page);
    // for(i=0;i<ap_count;i++){
    // cont = create_text(section, LV_SYMBOL_WIFI, ap_info[i].ssid, LV_MENU_ITEM_BUILDER_VARIANT_1);
    // lv_menu_set_load_page_event(menu, cont, NULL);
    // lv_group_add_obj(group,cont);}
    //     }
    //     else {
    //         wifi_stop();
    //     }
    }
}

static void BLK_Adjust_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * slider = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
    lcd_backlight_adjustment(200+lv_slider_get_value(slider)*8);
    }
}

static lv_obj_t * create_text(lv_obj_t * parent, const char * icon, const char * txt,
                              lv_menu_builder_variant_t builder_variant)
{
    lv_obj_t * obj = lv_menu_cont_create(parent);

    lv_obj_t * img = NULL;
    lv_obj_t * label = NULL;

    if(icon) {
        img = lv_img_create(obj);
        lv_img_set_src(img, icon);
    }

    if(txt) {
        label = lv_label_create(obj);
        lv_label_set_text(label, txt);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(label, 1);
    }

    if(builder_variant == LV_MENU_ITEM_BUILDER_VARIANT_2 && icon && txt) {
        lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
        lv_obj_swap(img, label);
    }

    return obj;
}

static lv_obj_t * create_slider(lv_obj_t * parent, const char * icon, const char * txt, int32_t min, int32_t max,
                                int32_t val)
{
    lv_obj_t * obj = create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_2);

    lv_obj_t * slider = lv_slider_create(obj);
    lv_obj_set_flex_grow(slider, 1);
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, val, LV_ANIM_ON);

    if(icon == NULL) {
        lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    }

    return obj;
}

static lv_obj_t * create_switch(lv_obj_t * parent, const char * icon, const char * txt, bool chk)
{
    lv_obj_t * obj = create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_1);

    lv_obj_t * sw = lv_switch_create(obj);
    lv_obj_add_state(sw, chk ? LV_STATE_CHECKED : 0);

    return obj;
}

#endif

