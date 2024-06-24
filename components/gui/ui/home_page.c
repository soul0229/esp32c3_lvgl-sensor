#include "core/lv_obj_pos.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "core/lv_obj.h"
#include "core/lv_obj_scroll.h"
#include "core/lv_obj_style.h"
#include "gui.h"
#include "lv_conf_internal.h"
#include "misc/lv_color.h"
#include "misc/lv_style.h"
#include "stdbool.h"
#include "mqtt.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
// #include "esp_wifi_types.h"

#if LV_USE_MENU && LV_USE_MSGBOX && LV_BUILD_EXAMPLES

extern lv_indev_t * indev_encoder;
extern lv_group_t * group;

static lv_obj_t * home_page = NULL;
static lv_obj_t * btn_back = NULL;
static lv_obj_t * label_tempture,* label_qiya,* label_shidu,* label_lighting;
static lv_obj_t * bar, *bar_label;
extern msg_queen_t mqtt_msg;
extern SemaphoreHandle_t xSemaphore[2];

static TaskHandle_t home_task,data_task;

extern const lv_img_dsc_t appicon;
extern const lv_img_dsc_t qiya32x32;
extern const lv_img_dsc_t tempture32x32;
extern const lv_img_dsc_t shidu32x32;
extern const lv_img_dsc_t lighting32x32;
extern const lv_img_dsc_t setting32x32;
static uint8_t target_open;
static uint8_t present_open = 0;

static void back_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        if(home_task != NULL){
            vTaskDelete(home_task);
            home_task = NULL;
        }
        if(data_task!= NULL){
            vTaskDelete(data_task);
            data_task = NULL;
        }
        lv_obj_del(home_page);
        lv_obj_del(btn_back);
        desktop_creat();
        group = NULL;
    }
}


static void home_page_update(void *pvParameter)
{
    uint8_t i = 0;
    uint8_t a, b, c, d;
    float para[8] = {0};
    while(1){
            if (xSemaphoreTake(xSemaphore[1], portMAX_DELAY) == pdTRUE){
                lv_label_set_text_fmt(label_tempture, "%d.%d", \
                    mqtt_msg.tempture[1][((mqtt_msg.cnt[1]-1)%MSQ_QUEEN_LEN)]/10, mqtt_msg.tempture[1][((mqtt_msg.cnt[1]-1)%MSQ_QUEEN_LEN)]%10);
                lv_label_set_text_fmt(label_qiya, "%d.%d", \
                    mqtt_msg.barometer[1][((mqtt_msg.cnt[1]-1)%MSQ_QUEEN_LEN)]/10, mqtt_msg.barometer[1][((mqtt_msg.cnt[1]-1)%MSQ_QUEEN_LEN)]%10);
                lv_label_set_text_fmt(label_shidu, "%d.%d", \
                    mqtt_msg.humidity[1][((mqtt_msg.cnt[1]-1)%MSQ_QUEEN_LEN)]/10, mqtt_msg.humidity[1][((mqtt_msg.cnt[1]-1)%MSQ_QUEEN_LEN)]%10);
                lv_label_set_text_fmt(label_lighting, "%d.%d", \
                    mqtt_msg.brightness[1][((mqtt_msg.cnt[1]-1)%MSQ_QUEEN_LEN)]/10, mqtt_msg.brightness[1][((mqtt_msg.cnt[1]-1)%MSQ_QUEEN_LEN)]%10);

                if(mqtt_msg.cnt[0]>MSQ_QUEEN_LEN && mqtt_msg.cnt[1]>MSQ_QUEEN_LEN){
                    for(i=0;i<8;i++)para[i] = 0;
                    for(i=0;i<MSQ_QUEEN_LEN;i++){
                        para[0] += mqtt_msg.tempture[0][i];
                        para[1] += mqtt_msg.tempture[1][i];
                        para[2] += mqtt_msg.barometer[0][i];
                        para[3] += mqtt_msg.barometer[1][i];
                        para[4] += mqtt_msg.humidity[0][i];
                        para[5] += mqtt_msg.humidity[1][i];
                        para[6] += mqtt_msg.brightness[0][i];
                        para[7] += mqtt_msg.brightness[1][i];
                    }
                    for(i=0;i<8;i++){
                        para[i] /= MSQ_QUEEN_LEN;
                    }
                    a = (para[0]-240) * (para[1]-para[0]);
                    if((abs(para[1]-para[0]) > 10) && (abs(para[1]-240)>10))a = -a;
                    else a=0;
                    b = (para[2]-1005) * (para[2]-para[3]);
                    if((abs(para[2]-para[3]) > 30) && (abs(para[3]-1005)>10))b = -b;
                    else b=0;
                    c = (para[4]-350) * (para[4]-para[5]);
                    if((abs(para[4]-para[5]) > 20) && (abs(para[5]-350)>10))c = -c;
                    else c=0;
                    d = (para[6]-2000) * (para[6]-para[7]);
                    if((abs(para[6]-para[7]) > 1000) && (abs(para[7]-1000)>100))d = -d;
                    else d=0;
                    target_open = (a*0.3+b*2+c*0.2+d*0.3)/(a+b+c+d+1)*100;
                    printf("a:%d,b:%d,c:%d,d:%d",a,b,c,d);
                    if(target_open>100)target_open=100;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }
    }
}

static void data_update(void *pvParameter)
{
    while(1){
        if(present_open > target_open)
            present_open--;
        else if(present_open < target_open)
            present_open++;
        lv_bar_set_value(bar, present_open, LV_ANIM_OFF);
        lv_label_set_text_fmt(bar_label, "%d", present_open);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

void home_page_creat(void)
{
    group = lv_group_create();
	lv_group_set_default(group);
	lv_indev_set_group(indev_encoder, group);

    /*Create a chart*/
    home_page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(home_page, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_center(home_page);
    lv_obj_clear_flag(home_page, LV_OBJ_FLAG_SCROLLABLE);
    

    lv_obj_t * sys_layer = lv_layer_sys();
    lv_obj_set_size(sys_layer, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    btn_back = lv_btn_create(sys_layer);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(btn_back, back_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_opa(btn_back, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(btn_back, &appicon, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(btn_back, 16, 16);

    lv_style_t style;
    lv_style_init(&style);
    lv_style_set_border_width(&style, 0);
    lv_obj_t *tempture = lv_obj_create(home_page);
    lv_obj_set_size(tempture, 30, 30);
    lv_obj_align(tempture, LV_ALIGN_LEFT_MID, 0, -25);
    lv_obj_set_style_bg_opa(tempture, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(tempture, &tempture32x32, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(tempture, &style, 0);
    label_tempture = lv_label_create(home_page);
    lv_obj_align_to(label_tempture, tempture, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_label_set_text(label_tempture, " ");

    lv_obj_t *shidu = lv_obj_create(home_page);
    lv_obj_set_size(shidu, 30, 30);
    lv_obj_align(shidu, LV_ALIGN_LEFT_MID, 0, 15);
    lv_obj_set_style_bg_opa(shidu, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(shidu, &shidu32x32, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(shidu, &style, 0);
    label_shidu = lv_label_create(home_page);
    lv_obj_align_to(label_shidu, shidu, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_label_set_text(label_shidu, " ");


    lv_obj_t *qiya = lv_obj_create(home_page);
    lv_obj_set_size(qiya, 30, 30);
    lv_obj_align(qiya, LV_ALIGN_RIGHT_MID, 0, -25);
    lv_obj_set_style_bg_opa(qiya, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(qiya, &qiya32x32, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(qiya, &style, 0);
    label_qiya = lv_label_create(home_page);
    lv_obj_align_to(label_qiya, qiya, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_label_set_text(label_qiya, " ");

    lv_obj_t *lighting = lv_obj_create(home_page);
    lv_obj_set_size(lighting, 30, 30);
    lv_obj_align(lighting, LV_ALIGN_RIGHT_MID, 0, 15);
    lv_obj_set_style_bg_opa(lighting, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(lighting, &lighting32x32, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(lighting, &style, 0);
    label_lighting = lv_label_create(home_page);
    lv_obj_align_to(label_lighting, lighting, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_label_set_text(label_lighting, " ");
    if(mqtt_msg.cnt[1] > 0){
        lv_label_set_text_fmt(label_tempture, "%d.%d", \
                        mqtt_msg.tempture[1][((mqtt_msg.cnt[1]-1)%MSQ_QUEEN_LEN)]/10, mqtt_msg.tempture[1][((mqtt_msg.cnt[1]-1)%MSQ_QUEEN_LEN)]%10);
        lv_label_set_text_fmt(label_qiya, "%d.%d", \
                        mqtt_msg.barometer[1][((mqtt_msg.cnt[1]-1)%MSQ_QUEEN_LEN)]/10, mqtt_msg.barometer[1][((mqtt_msg.cnt[1]-1)%MSQ_QUEEN_LEN)]%10);
        lv_label_set_text_fmt(label_shidu, "%d.%d", \
                        mqtt_msg.humidity[1][((mqtt_msg.cnt[1]-1)%MSQ_QUEEN_LEN)]/10, mqtt_msg.humidity[1][((mqtt_msg.cnt[1]-1)%MSQ_QUEEN_LEN)]%10);
        lv_label_set_text_fmt(label_lighting, "%d.%d", \
                        mqtt_msg.brightness[1][((mqtt_msg.cnt[1]-1)%MSQ_QUEEN_LEN)]/10, mqtt_msg.brightness[1][((mqtt_msg.cnt[1]-1)%MSQ_QUEEN_LEN)]%10);
    }
    bar = lv_bar_create(home_page);
    lv_obj_set_size(bar, 60, 20);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, 0);
    bar_label = lv_label_create(home_page);
    lv_label_set_text(bar_label, "0");
    lv_obj_align_to(bar_label, bar, LV_ALIGN_OUT_TOP_MID, 0, 0);    /*Align top of the slider*/
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    xTaskCreatePinnedToCore(home_page_update, "home page", 2048, NULL, 9, &home_task, 0);
    xTaskCreatePinnedToCore(data_update, "data update", 1024, NULL, 9, &data_task, 0);

}

#endif