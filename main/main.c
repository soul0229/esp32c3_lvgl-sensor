/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/semphr.h"

#include "blufi_main.h"
#include "bmp280.h"
#include "ath20.h"
#include "bh1750.h"
#include "lv_api_map.h"
#include "mqtt.h"
#include "temt6000.h"

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "gui.h"

#define TAG "demo"
#define LV_TICK_PERIOD_MS 1

#define BLINK_GPIO 13
static uint8_t s_led_state = 0;
static SemaphoreHandle_t lvgl_mux = NULL;

bool example_lvgl_lock(int timeout_ms)
{
    // Convert timeout in milliseconds to FreeRTOS ticks
    // If `timeout_ms` is set to -1, the program will block until the condition is met
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

void example_lvgl_unlock(void)
{
    xSemaphoreGiveRecursive(lvgl_mux);
}

extern void example_lvgl_demo_ui(lv_disp_t *disp);



/**********************
 *  STATIC PROTOTYPES
 **********************/
static void guiTask(void *pvParameter);

void app_main(void)
{
    mqtt_main();
#if CONFIG_CONTROL_DEVICE
    blufi_init();
#else
    uint8_t ret_t = 0;
    ret_t = ATH20_Init();
    if(ret_t == 0)
    {
        printf("ATH20传感器初始化错误!\n");
        while(1)vTaskDelay(30 / portTICK_PERIOD_MS);
    }

    ret_t = bmp280_init();
    if(ret_t != 0x58)
    {
        printf("BMP280传感器初始化错误!\n");
        while(1)vTaskDelay(30 / portTICK_PERIOD_MS);
    }
    ret_t = BH1750_Init();
    if(ret_t == 0)
    {
        printf("BH1750传感器初始化错误!\n");
        while(1)vTaskDelay(30 / portTICK_PERIOD_MS);
    }
    wifi_connect();    
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    my_mqtt_start();
#endif


#if CONFIG_CONTROL_DEVICE
    lv_port_init();
    lv_port_indev_init();
    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(lvgl_mux);
    ESP_LOGI(TAG, "Create LVGL task");
    xTaskCreatePinnedToCore(guiTask, "gui", 4096, NULL, 10, NULL, 0);
    ESP_LOGI(TAG, "Display LVGL Meter Widget");
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (example_lvgl_lock(-1)) {
        // ui_init();
        home_page_creat();
        // desktop_creat();
        // example_lvgl_demo_ui(disp);
        // setting_creat();
        // lv_demo_music();
        // lv_demo_benchmark();
        // Release the mutex
        example_lvgl_unlock();
    }
#endif
}

static void guiTask(void *pvParameter) {

    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
        // while(1){
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
        // s_led_state = !s_led_state;
        // gpio_set_level(BLINK_GPIO, s_led_state);
        // }

    ESP_LOGI(TAG, "Starting LVGL task");
    while (1) {
        // Lock the mutex due to the LVGL APIs are not thread-safe
        if (example_lvgl_lock(-1)) {
            lv_timer_handler();
            // Release the mutex
            example_lvgl_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
}