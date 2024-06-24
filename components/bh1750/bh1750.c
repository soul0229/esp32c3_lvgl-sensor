/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "i2c_common.h"
#include "bh1750.h"

#define SCL_IO_PIN CONFIG_I2C_MASTER_SCL
#define SDA_IO_PIN CONFIG_I2C_MASTER_SDA
#define MASTER_FREQUENCY CONFIG_I2C_MASTER_FREQUENCY
#define PORT_NUMBER -1

i2c_device_handle_t bh1750_handle;

float Multiple_Read_BH1750()
{
    uint8_t buf[2];
    int16_t data;
    uint8_t write_buf[] = {POWER_ON, HRES_MODE};

    i2c_master_transmit(bh1750_handle->i2c_dev, write_buf, 1, -1);
    i2c_master_transmit(bh1750_handle->i2c_dev, &write_buf[1], 1, -1);
    vTaskDelay(180 / portTICK_PERIOD_MS);

    i2c_master_receive(bh1750_handle->i2c_dev, buf, 2, -1);
    // i2c_device_read(bh1750_handle, NULL, buf, 3);
    data = buf[0];
    data = (data << 8) + buf[1];//合成数据，即光照数据
    printf("buf[0]:%d buf[1]:%d \n", buf[0], buf[1]);
    return (float)data/1.2;
}
                                                            
uint8_t BH1750_Init(void)
{

    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = PORT_NUMBER,
        .scl_io_num = SCL_IO_PIN,
        .sda_io_num = SDA_IO_PIN,
        .glitch_ignore_cnt = 7,
    };
    
    extern i2c_master_bus_handle_t bus_handle;
    if(bus_handle == NULL)
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));

    i2c_sensor_config_t bh1750_config = {
        .device.scl_speed_hz = MASTER_FREQUENCY,
        .device.device_address = BH1750_SLAVE_ADDRESS,
        .addr_wordlen = 1,
        .write_time_ms = 10,
    };

    ESP_ERROR_CHECK(i2c_device_init(bus_handle, &bh1750_config, &bh1750_handle));
    vTaskDelay(40 / portTICK_PERIOD_MS);
    uint8_t write_buf[] = {POWER_ON, HRES_MODE};

    i2c_master_transmit(bh1750_handle->i2c_dev, write_buf, 1, -1);
    vTaskDelay(180 / portTICK_PERIOD_MS);

    return 1;
}
