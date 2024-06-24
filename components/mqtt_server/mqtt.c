/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "mqtt.h"
#include "bmp280.h"
#include "ath20.h"
#include "bh1750.h"

static const char *TAG = "mqtt.c";

#define VERSION_REPORT          "{\"params\":{\"tempture\":\"%d\",\"humidity\":\"%d\",\"barometer\":\"%d\",\"brightness\":\"%d\"}}"

msg_queen_t mqtt_msg = {0};
SemaphoreHandle_t xSemaphore[2];
static esp_mqtt_client_handle_t client = NULL;

#ifndef CONFIG_CONTROL_DEVICE
static char code_buff[256];
static float P,T,ALT;
static uint32_t CT_data[2];
static int  c1,t1;
static float lx;
#endif

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void request_data(void *pvParameter)
{
    while(1){
        esp_mqtt_client_publish(client, "control", "{\"requst\":\"1\"}", 0, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    char* pos;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
#if CONFIG_CONTROL_DEVICE
        msg_id = esp_mqtt_client_subscribe(client, "indoor", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        msg_id = esp_mqtt_client_subscribe(client, "outdoor", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        xTaskCreatePinnedToCore(request_data, "request_data", 1024, NULL, 9, NULL, 0);
#else
        msg_id = esp_mqtt_client_subscribe(client, "control", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
#endif
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // msg_id = esp_mqtt_client_publish(client, "esp32c3", "data9021", 0, 0, 0);
        // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        #if CONFIG_CONTROL_DEVICE
        if(!strncmp(event->topic, "outdoor", event->topic_len)){
            pos = strstr((char *)event->data, "\"tempture\"");
             if (pos != NULL) {
            sscanf(pos, "\"tempture\":\"%d\"", &mqtt_msg.tempture[0][mqtt_msg.cnt[0]%64]);
            }
            pos = strstr((char *)event->data, "\"humidity\"");
            if (pos != NULL) {
            sscanf(pos, "\"humidity\":\"%d\"", &mqtt_msg.humidity[0][mqtt_msg.cnt[0]%64]);
            }
            pos = strstr((char *)event->data, "\"barometer\"");
            if (pos != NULL) {
            sscanf(pos, "\"barometer\":\"%d\"", &mqtt_msg.barometer[0][mqtt_msg.cnt[0]%64]);
            }
            pos = strstr((char *)event->data, "\"brightness\"");
            if (pos != NULL) {
            sscanf(pos, "\"brightness\":\"%d\"", &mqtt_msg.brightness[0][mqtt_msg.cnt[0]%64]);
            }
            mqtt_msg.cnt[0]++;
            xSemaphoreGive(xSemaphore[0]);
        }
        else if(!strncmp(event->topic, "indoor", event->topic_len)){
            pos = strstr((char *)event->data, "\"tempture\"");
             if (pos != NULL) {
            sscanf(pos, "\"tempture\":\"%d\"", &mqtt_msg.tempture[1][mqtt_msg.cnt[1]%64]);
            }
            pos = strstr((char *)event->data, "\"humidity\"");
            if (pos != NULL) {
            sscanf(pos, "\"humidity\":\"%d\"", &mqtt_msg.humidity[1][mqtt_msg.cnt[1]%64]);
            }
            pos = strstr((char *)event->data, "\"barometer\"");
            if (pos != NULL) {
            sscanf(pos, "\"barometer\":\"%d\"", &mqtt_msg.barometer[1][mqtt_msg.cnt[1]%64]);
            }
            pos = strstr((char *)event->data, "\"brightness\"");
            if (pos != NULL) {
            sscanf(pos, "\"brightness\":\"%d\"", &mqtt_msg.brightness[1][mqtt_msg.cnt[1]%64]);
            }
            mqtt_msg.cnt[1]++;
            xSemaphoreGive(xSemaphore[1]);
        }
        #else
            if(!strncmp(event->topic, "control", event->topic_len)){

                /* 读取 ATH20 传感器数据*/
                while(ATH20_Read_Cal_Enable() == 0)
                {
                    ATH20_Init();//如果为0再使能一次
                    vTaskDelay(30 / portTICK_PERIOD_MS);
                }
                ATH20_Read_CTdata(CT_data);  //读取温度和湿度
                c1 = CT_data[0] * 1000 / 1024 / 1024;  //计算得到湿度值(放大了10倍,如果c1=523,表示现在湿度为52.3%)
                t1 = CT_data[1] * 200 *10 / 1024 / 1024 - 500;//计算得到温度值(放大了10倍,如果t1=245,表示现在温度为24.5摄氏度)

                /* 读取 BMP280 传感器数据*/
                BMP280GetData(&P, &T, &ALT);
                lx = Multiple_Read_BH1750();

                printf("***************************\n");
                printf("AHT20传感器数据:\n");
                printf("温度: %d.%d 摄氏度\n",(t1/10),(t1%10));
                printf("湿度: %d.%d %%\n",(c1/10),(c1%10));
                printf("BMP280传感器数据:\n");
                printf("气压: %0.4f hPa\n",P);
                printf("温度: %0.2f 摄氏度\n",T);
                printf("海拔: %0.2f m\n",ALT);
                printf("BH1750传感器数据:\n");
                printf("亮度: %0.4f lx\n",lx);
                printf("\n\n");
                sprintf(code_buff, VERSION_REPORT, (int)((t1+T*10)/2), c1, (int)P, (int)lx);
                printf(code_buff);
                esp_mqtt_client_publish(client, "indoor", code_buff, 0, 0, 0);
            }
        #endif
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
        .credentials.client_id = CONFIG_CLIENT_ID,
    };
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.broker.address.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.broker.address.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    // esp_mqtt_client_start(client);
    xSemaphore[0] = xSemaphoreCreateBinary();
    xSemaphore[1] = xSemaphoreCreateBinary();
}

void mqtt_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    mqtt_app_start();
}
void my_mqtt_start(void)
{
    esp_mqtt_client_start(client);
}
void my_mqtt_stop(void)
{
    esp_mqtt_client_stop(client);
}

