#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// The pixel number in horizontal and vertical
#define LCD_H_RES              160
#define LCD_V_RES              80
// Bit number used to represent command and parameter
#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

#ifdef CONFIG_IDF_TARGET_ESP32
#define LCD_HOST        (HSPI_HOST)
#define SCLK_PIN        (14)
#define MOSI_PIN        (13)
#define CS_PIN          (15)

#define DC_PIN          (2)
#define RST_PIN         (12)
#define LCD_BCKL_PIN    (4) // Define the output GPIO

#elif defined CONFIG_IDF_TARGET_ESP32C3
#define LCD_HOST        (SPI2_HOST)
#define SCLK_PIN        (2)
#define MOSI_PIN        (3)
#define CS_PIN          (7)

#define DC_PIN          (6)
#define RST_PIN         (10)
#define LCD_BCKL_PIN    (11) // Define the output GPIO
#endif

#define SPI_CLK         (60*1000*1000)
#define LCD_DIRECTION   (90)

#define LVGL_TICK_PERIOD_MS    1

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_10_BIT // Set duty resolution to 10 bits
#define LEDC_DUTY               (0) // Set duty to 0%. ((2 ** 10) - 1) * 0% = 0
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz

lv_disp_t* lv_port_init(void);
void lcd_backlight_adjustment(uint16_t duty);

#ifdef __cplusplus
}
#endif
