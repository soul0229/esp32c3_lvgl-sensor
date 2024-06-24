/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "i2c_common.h"
#include "ath20.h"

#define SCL_IO_PIN CONFIG_I2C_MASTER_SCL
#define SDA_IO_PIN CONFIG_I2C_MASTER_SDA
#define MASTER_FREQUENCY CONFIG_I2C_MASTER_FREQUENCY
#define PORT_NUMBER -1

i2c_device_handle_t ath20_handle;

uint8_t ATH20_Read_Status(void) //读取AHT10的状态寄存器
{
    uint8_t Byte_first;
    
    i2c_device_read(ath20_handle, 0x00, (uint8_t *)&Byte_first, 1);
	return Byte_first;
}

uint8_t ATH20_Read_Cal_Enable(void)
{
    uint8_t val = 0;//ret = 0,

    val = ATH20_Read_Status();
    if((val & 0x68) == 0x08)  //判断NOR模式和校准输出是否有效
        return 1;
    else
        return 0;
}

void ATH20_Read_CTdata(uint32_t *ct) //读取AHT10的温度和湿度数据
{
    uint32_t RetuData = 0;
	uint16_t cnt = 0;
    uint8_t Data[10];
    uint8_t tmp[10];

    tmp[0] = 0x33;
    tmp[1] = 0x00;
    i2c_device_write(ath20_handle, StartTest, tmp, 2);
	vTaskDelay(75 / portTICK_PERIOD_MS);

    cnt = 0;
	while(((ATH20_Read_Status()&0x80) == 0x80))//等待忙状态结束
	{
        vTaskDelay(1 / portTICK_PERIOD_MS);
        if(cnt++ >= 100)
        {
            break;
        }
	}

    i2c_device_read(ath20_handle, 0x00, (uint8_t *)&Data, 7);

	RetuData = 0;
    RetuData = (RetuData|Data[1]) << 8;
	RetuData = (RetuData|Data[2]) << 8;
	RetuData = (RetuData|Data[3]);
	RetuData = RetuData >> 4;
	ct[0] = RetuData;

    RetuData = 0;
	RetuData = (RetuData|Data[3]) << 8;
	RetuData = (RetuData|Data[4]) << 8;
	RetuData = (RetuData|Data[5]);
	RetuData = RetuData&0xfffff;
	ct[1] = RetuData;
}

static uint8_t count;
uint8_t ATH20_Init(void)
{
    uint8_t tmp[10];

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

    i2c_sensor_config_t ath20_config = {
        .device.scl_speed_hz = MASTER_FREQUENCY,
        .device.device_address = ATH20_SLAVE_ADDRESS,
        .addr_wordlen = 1,
        .write_time_ms = 10,
    };

    ESP_ERROR_CHECK(i2c_device_init(bus_handle, &ath20_config, &ath20_handle));

    vTaskDelay(40 / portTICK_PERIOD_MS);

    tmp[0] = 0x08;
    tmp[1] = 0x00;

    i2c_device_write(ath20_handle, INIT, tmp, 2);

    vTaskDelay(500 / portTICK_PERIOD_MS);
    count = 0;

    while(ATH20_Read_Cal_Enable() == 0)////需要等待状态字status的Bit[3]=1时才去读数据。如果Bit[3]不等于1 ，发软件复位0xBA给AHT10，再重新初始化AHT10，直至Bit[3]=1
    {
        i2c_device_write(ath20_handle, SoftReset, tmp, 1);
        vTaskDelay(200 / portTICK_PERIOD_MS);

        i2c_device_write(ath20_handle, INIT, tmp, 2);

        count++;
        if(count >= 10)
            return 0;
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    return 1;
}
