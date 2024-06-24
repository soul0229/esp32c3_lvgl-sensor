#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "core/lv_disp.h"
#include "core/lv_obj.h"
#include "gui.h"
#include "lv_api_map.h"
#include "lv_conf_internal.h"
#include "stdbool.h"
#include "mqtt.h"
#include <stdint.h>

#if LV_USE_CHART
extern msg_queen_t mqtt_msg;
extern lv_indev_t * indev_encoder;
extern lv_group_t * group;
extern SemaphoreHandle_t xSemaphore[2];

static TaskHandle_t chart_task[2];
static TaskHandle_t bar_task[2];


static lv_obj_t * chart_page = NULL;
static lv_obj_t *chart = NULL;
static lv_obj_t *bar = NULL;
static lv_obj_t * btn_back = NULL;
static lv_obj_t * btn_switch = NULL;
static int *which_para = NULL;
static int min, max;
extern const lv_img_dsc_t wallpaperbar;

static void chart_update(void *pvParameter)
{
    lv_chart_series_t * ser = (lv_chart_series_t *)pvParameter;
    uint8_t which_device = (xTaskGetCurrentTaskHandle() == chart_task[1]);
    while(1){
        if (xSemaphoreTake(xSemaphore[which_device], portMAX_DELAY) == pdTRUE){
            lv_chart_set_next_value(chart, ser, \
                (lv_coord_t )*(which_para + which_device*MSQ_QUEEN_LEN + ((mqtt_msg.cnt[which_device] - 1)%MSQ_QUEEN_LEN)));
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

static void chart_creat(void)
{
    uint16_t cnt = 0;
    uint8_t which_device = 0;
    char thread_name[16];
    lv_chart_series_t * ser[2];

    chart = lv_chart_create(chart_page);
    lv_obj_set_size(chart, 124, 80);
    lv_obj_align_to(chart, chart_page, LV_ALIGN_CENTER, 18, 0);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, min, max);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 10, 5, 6, 2, true, 50);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);

    /*Do not display points on the data*/
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);

    ser[0] = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    ser[1] = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_point_count(chart, MSQ_QUEEN_LEN);

    while(which_device <= 1){
        cnt = (mqtt_msg.cnt[which_device] < MSQ_QUEEN_LEN)?0:(mqtt_msg.cnt[which_device]-MSQ_QUEEN_LEN);
        while(cnt < mqtt_msg.cnt[which_device]){
            lv_chart_set_next_value(chart, ser[which_device], (lv_coord_t )*(which_para + which_device*MSQ_QUEEN_LEN + (cnt%MSQ_QUEEN_LEN)));
            cnt++;
        }
        sprintf(thread_name, "chart_update%d", which_device);
        xTaskCreatePinnedToCore(chart_update, thread_name, 1024, ser[which_device], 9, &chart_task[which_device], 0);
        which_device++;
    }

}

static void bar_update(void *pvParameter)
{
    lv_obj_t * bar = (lv_obj_t * )pvParameter;
    uint8_t which_device = (xTaskGetCurrentTaskHandle() == bar_task[1]);
    while(1){
        if (xSemaphoreTake(xSemaphore[which_device], portMAX_DELAY) == pdTRUE){
            lv_bar_set_value(bar, *(which_para + which_device*MSQ_QUEEN_LEN + ((mqtt_msg.cnt[which_device] - 1)%MSQ_QUEEN_LEN)), LV_ANIM_ON);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

static void event_cb(lv_event_t * e)
{
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    if(dsc->part != LV_PART_INDICATOR) return;

    lv_obj_t * obj = lv_event_get_target(e);

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.font = LV_FONT_DEFAULT;

    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d", (int)lv_bar_get_value(obj));

    lv_point_t txt_size;
    lv_txt_get_size(&txt_size, buf, label_dsc.font, label_dsc.letter_space, label_dsc.line_space, LV_COORD_MAX,
                    label_dsc.flag);

    lv_area_t txt_area;
    /*If the indicator is long enough put the text inside on the right*/
    if(lv_area_get_width(dsc->draw_area) > txt_size.x + 20) {
        txt_area.x2 = dsc->draw_area->x2 - 5;
        txt_area.x1 = txt_area.x2 - txt_size.x + 1;
        label_dsc.color = lv_color_white();
    }
    /*If the indicator is still short put the text out of it on the right*/
    else {
        txt_area.x1 = dsc->draw_area->x2 + 5;
        txt_area.x2 = txt_area.x1 + txt_size.x - 1;
        label_dsc.color = lv_color_black();
    }

    txt_area.y1 = dsc->draw_area->y1 + (lv_area_get_height(dsc->draw_area) - txt_size.y) / 2;
    txt_area.y2 = txt_area.y1 + txt_size.y - 1;

    lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area, buf, NULL);
}

static void bar_creat(void)
{
    uint8_t which_device = 0;
    char thread_name[16];
    static lv_style_t style_indic;
    lv_obj_t * Tbar[2];

    lv_style_init(&style_indic);
    lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
    lv_style_set_bg_color(&style_indic, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_bg_grad_color(&style_indic, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_bg_grad_dir(&style_indic, LV_GRAD_DIR_HOR);

    bar = lv_obj_create(chart_page);
    lv_obj_set_style_bg_img_src(bar, &wallpaperbar, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(bar, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_center(bar);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    Tbar[0] = lv_bar_create(bar);
    lv_obj_add_style(Tbar[0], &style_indic, LV_PART_INDICATOR);
    lv_obj_set_size(Tbar[0], 100, 15);
    lv_obj_align_to(Tbar[0], chart_page, LV_ALIGN_TOP_MID, 10, 5);
    lv_bar_set_range(Tbar[0], min, max);
    lv_obj_add_event_cb(Tbar[0], event_cb, LV_EVENT_DRAW_PART_END, NULL);

    Tbar[1] = lv_bar_create(bar);
    lv_obj_add_style(Tbar[1], &style_indic, LV_PART_INDICATOR);
    lv_obj_set_size(Tbar[1], 100, 15);
    lv_obj_align_to(Tbar[1], chart_page, LV_ALIGN_BOTTOM_MID, 10, 0);
    lv_bar_set_range(Tbar[1], min, max);
    lv_obj_add_event_cb(Tbar[1], event_cb, LV_EVENT_DRAW_PART_END, NULL);

    while(which_device <= 1){
        if(mqtt_msg.cnt[which_device] > 0)
            lv_bar_set_value(Tbar[which_device], *(which_para + which_device*MSQ_QUEEN_LEN + ((mqtt_msg.cnt[which_device] - 1)%MSQ_QUEEN_LEN)), LV_ANIM_ON);
        sprintf(thread_name, "bar_update%d", which_device);
        xTaskCreatePinnedToCore(bar_update, thread_name, 1024, Tbar[which_device], 9, &bar_task[which_device], 0);
        which_device++;
    }

}


static void switch_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    uint8_t i = 0;
    if(code == LV_EVENT_CLICKED) {
        if(chart != NULL){
            while(i < 2){
                if(chart_task[i] != NULL){
                    vTaskDelete(chart_task[i]);
                    chart_task[i] = NULL;
                }
                i++;
            }
            lv_obj_del(chart);
            bar_creat();
            chart = NULL;
        }
        else if(bar != NULL){
            while(i < 2){
                if(bar_task[i] != NULL){
                    vTaskDelete(bar_task[i]);
                    bar_task[i] = NULL;
                }
                i++;
            }
            lv_obj_del(bar);
            chart_creat();
            bar = NULL;
        }
    }
}


static void back_event_handler(lv_event_t * e)
{
    uint8_t i = 0;
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        while(i < 2){
            if(bar_task[i] != NULL){
                vTaskDelete(bar_task[i]);
                bar_task[i] = NULL;
            }
            if(chart_task[i] != NULL){
                vTaskDelete(chart_task[i]);
                chart_task[i] = NULL;
            }
            i++;
        }
        lv_obj_del(chart_page);
        lv_obj_del(btn_back);
        lv_obj_del(btn_switch);
        desktop_creat();
        group = NULL;
    }
}

/**
 * Display 1000 data points with zooming and scrolling.
 * See how the chart changes drawing mode (draw only vertical lines) when
 * the points get too crowded.
 */
void chart_temp_creat(int *parameter, int para_min, int para_max)
{
    group = lv_group_create();
	lv_group_set_default(group);
	lv_indev_set_group(indev_encoder, group);

    which_para = parameter;
    min = para_min;
    max = para_max;

    /*Create a chart*/
    chart_page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(chart_page, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_center(chart_page);
    lv_obj_clear_flag(chart_page, LV_OBJ_FLAG_SCROLLABLE);
    

    lv_obj_t * sys_layer = lv_layer_sys();
    lv_obj_set_size(sys_layer, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    btn_back = lv_btn_create(sys_layer);
    lv_obj_align(btn_back, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_add_event_cb(btn_back, back_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_t * label = lv_label_create(btn_back);
    lv_label_set_text(label, LV_SYMBOL_CLOSE);
    const lv_font_t * back_font = lv_obj_get_style_text_font(btn_back, LV_PART_MAIN);
    lv_coord_t back_btn_size = lv_font_get_line_height(back_font) + LV_DPX(2);
    lv_obj_set_size(btn_back, back_btn_size, back_btn_size);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);


    btn_switch = lv_btn_create(sys_layer);
    lv_obj_add_event_cb(btn_switch, switch_btn_event_cb, LV_EVENT_CLICKED, NULL);
    label = lv_label_create(btn_switch);
    lv_label_set_text(label, LV_SYMBOL_REFRESH);
    const lv_font_t * switch_font = lv_obj_get_style_text_font(btn_switch, LV_PART_MAIN);
    lv_coord_t switch_btn_size = lv_font_get_line_height(switch_font) + LV_DPX(4);
    lv_obj_set_size(btn_switch, switch_btn_size, switch_btn_size);
    lv_obj_align(btn_switch, LV_ALIGN_TOP_RIGHT, -switch_btn_size-LV_DPX(4), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);


    chart_creat();

}

#endif