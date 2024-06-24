/**
 * @file lv_port_indev_templ.c
 *
 */

/*Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
/*********************
 *      DEFINES
 *********************/

#ifdef CONFIG_IDF_TARGET_ESP32
#define GPIO_INPUT_IO_0     32
#define GPIO_INPUT_IO_1     35
#define GPIO_INPUT_IO_2     34

#elif defined CONFIG_IDF_TARGET_ESP32C3
#define GPIO_INPUT_IO_0     5
#define GPIO_INPUT_IO_1     9
#define GPIO_INPUT_IO_2     4

#define GPIO_INPUT_IO_3     8
#define GPIO_INPUT_IO_4     13
#endif

#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1) | (1ULL<<GPIO_INPUT_IO_2) | (1ULL<<GPIO_INPUT_IO_3) | (1ULL<<GPIO_INPUT_IO_4))
#define ESP_INTR_FLAG_DEFAULT 0
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static const char *TAG = "lv_port_indev";

static void encoder_init(void);
static void encoder_with_keys_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
void encoder_handler(void);

static void gpio_isr_handler(void* arg);
/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_encoder;
lv_group_t * group = NULL;

static int32_t encoder_diff;
static lv_indev_state_t encoder_state;

static QueueHandle_t gpio_evt_queue = NULL;
static key_read key;
static uint32_t gpio_num;
/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    /**
     * Here you will find example implementation of input devices supported by LittelvGL:
     *  - Touchpad
     *  - Mouse (with cursor support)
     *  - Keypad (supports GUI usage only with key)
     *  - Encoder (supports GUI usage only with: left, right, push)
     *  - Button (external buttons to press points on the screen)
     *
     *  The `..._read()` function are only examples.
     *  You should shape them according to your hardware
     */

    static lv_indev_drv_t indev_drv;

    /*------------------
     * Encoder
     * -----------------*/

    /*Initialize your encoder if you have*/
    encoder_init();

    /*Register a encoder input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = encoder_with_keys_read;
    indev_encoder = lv_indev_drv_register(&indev_drv);

    /*Later you should create group(s) with `lv_group_t * group = lv_group_create()`,
     *add objects to the group with `lv_group_add_obj(group, obj)`
     *and assign this input device to group to navigate in it:
     *`lv_indev_set_group(indev_encoder, group);`*/
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Encoder
 * -----------------*/

/*Initialize your keypad*/
static void encoder_init(void)
{
    /*Your code comes here*/
    ESP_LOGI(TAG, "Initialize encoder");
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);

    gpio_isr_handler_add(GPIO_INPUT_IO_2, gpio_isr_handler, (void*) GPIO_INPUT_IO_2);

    gpio_isr_handler_add(GPIO_INPUT_IO_3, gpio_isr_handler, (void*) GPIO_INPUT_IO_3);

    gpio_isr_handler_add(GPIO_INPUT_IO_4, gpio_isr_handler, (void*) GPIO_INPUT_IO_4);
}

int16_t enc_get_new_moves(void)
{
	int16_t encoder_diff;
	if(key.last_key==LV_KEY_UP&&gpio_get_level(key.GPIO_Pin)==0) encoder_diff = 1;
	else if(key.last_key==LV_KEY_DOWN&&gpio_get_level(key.GPIO_Pin)==0) encoder_diff = -1;
	else encoder_diff = 0;
	return encoder_diff;
}

/*Will be called by the library to read the encoder*/
static void encoder_with_keys_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
  data->key = key.last_key;            /*Get the last pressed or released key*/
                                     /* use LV_KEY_ENTER for encoder press */
  if(!gpio_get_level(key.GPIO_Pin)) data->state = LV_INDEV_STATE_PRESSED;
  else {
      data->state = LV_INDEV_STATE_RELEASED;
      /* Optionally you can also use enc_diff, if you have encoder*/
      data->enc_diff = enc_get_new_moves();
  }
}

/*Call this function in an interrupt to process encoder events (turn, press)*/
void encoder_handler(void)
{
    /*Your code comes here*/

    encoder_diff += 0;
    encoder_state = LV_INDEV_STATE_REL;
}

static void gpio_isr_handler(void* arg)
{
    gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    if(gpio_num==GPIO_INPUT_IO_0){
		key.last_key = LV_KEY_LEFT;
		key.GPIO_Pin = GPIO_INPUT_IO_0;
    }
    else if (gpio_num==GPIO_INPUT_IO_1)
    {
        key.last_key = LV_KEY_RIGHT;
		key.GPIO_Pin = GPIO_INPUT_IO_1;
    }
    else if (gpio_num==GPIO_INPUT_IO_2)
    {
        key.last_key = LV_KEY_ENTER;
		key.GPIO_Pin = GPIO_INPUT_IO_2;
    }
    else if (gpio_num==GPIO_INPUT_IO_3)
    {
        key.last_key = LV_KEY_UP;
		key.GPIO_Pin = GPIO_INPUT_IO_3;
    }
    else if (gpio_num==GPIO_INPUT_IO_4)
    {
        key.last_key = LV_KEY_DOWN;
		key.GPIO_Pin = GPIO_INPUT_IO_4;
    }
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
