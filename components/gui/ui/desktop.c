#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "core/lv_obj_scroll.h"
#include "gui.h"
#include "lv_conf_internal.h"
#include "misc/lv_color.h"
#include "stdbool.h"
#include "mqtt.h"
#include <stdint.h>
// #include "esp_wifi_types.h"

#if LV_USE_MENU && LV_USE_MSGBOX && LV_BUILD_EXAMPLES

extern lv_indev_t * indev_encoder;
extern lv_group_t * group;

static lv_obj_t * desktop = NULL;
extern msg_queen_t mqtt_msg;
extern const lv_img_dsc_t qiya;
extern const lv_img_dsc_t tempture;
extern const lv_img_dsc_t shidu;
extern const lv_img_dsc_t lighting;
extern const lv_img_dsc_t setting;
extern const lv_img_dsc_t home;
extern const lv_img_dsc_t wallpaper1, wallpaper2, wallpaper3, wallpaper4, wallpaper5, wallpaper6, wallpaper7, wallpaper8, wallpaper9, wallpaper10, wallpaperbar;

static const lv_img_dsc_t *image_t[] = {&home,& qiya, &tempture, &shidu, &lighting, &setting};
const lv_img_dsc_t *wallpaper[] = {&wallpaper1, &wallpaper2, &wallpaper3, &wallpaper4, &wallpaper5, &wallpaper6, &wallpaper7, &wallpaper8, &wallpaper9, &wallpaper10, &wallpaperbar};



static void btn_qiya_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        lv_obj_del(desktop);
        chart_temp_creat((int*)mqtt_msg.barometer, 350, 1040);
        group = NULL;
    }
}

static void btn_tempture_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        lv_obj_del(desktop);
        chart_temp_creat((int*)mqtt_msg.tempture, -100, 500);
        group = NULL;
    }
}

static void btn_shidu_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        lv_obj_del(desktop);
        chart_temp_creat((int*)mqtt_msg.humidity, 0, 1000);
        group = NULL;
    }
}

static void btn_guangzhao_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        lv_obj_del(desktop);
        chart_temp_creat((int*)mqtt_msg.brightness, 0, 6000);
        group = NULL;
    }
}
static void btn_setting_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        lv_obj_del(desktop);
        setting_creat();
        group = NULL;
    }
}
static void btn_home_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        lv_obj_del(desktop);
        home_page_creat();
        group = NULL;
    }
}

static lv_event_cb_t btn_event_cb[] = {
    btn_home_cb,
    btn_qiya_cb,
    btn_tempture_cb,
    btn_shidu_cb,
    btn_guangzhao_cb,
    btn_setting_cb,
};


/**
 * Show an example to scroll snap
 */
void desktop_creat(void)
{
    static uint8_t wallpaper_cnt = 0;
    group = lv_group_create();
	lv_group_set_default(group);
	lv_indev_set_group(indev_encoder, group);

    lv_disp_t * display = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(display, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                               false, LV_FONT_DEFAULT);
    lv_disp_set_theme(display, theme);
    lv_obj_set_scrollbar_mode(lv_scr_act(), LV_SCROLLBAR_MODE_OFF);

    desktop = lv_obj_create(lv_scr_act());
    lv_obj_set_size(desktop, 320, 84);
    lv_obj_set_scrollbar_mode(desktop, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_snap_x(desktop, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_flex_flow(desktop, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_img_src(desktop, wallpaper[wallpaper_cnt%(sizeof(wallpaper)/sizeof(wallpaper[0]))], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(desktop, LV_ALIGN_CENTER, 0, 0);
    wallpaper_cnt++;

    uint32_t i;
    for(i = 0; i < sizeof(btn_event_cb)/sizeof(btn_event_cb[0]); i++) {
        lv_obj_t * btn = lv_btn_create(desktop);
        lv_obj_set_size(btn, 48, 48);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_bg_img_src(btn, image_t[i], LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(btn, btn_event_cb[i], LV_EVENT_ALL, NULL);
    }
}


#endif

