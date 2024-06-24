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
#include "bmp280.h"
#include <math.h>

#define SCL_IO_PIN CONFIG_I2C_MASTER_SCL
#define SDA_IO_PIN CONFIG_I2C_MASTER_SDA
#define MASTER_FREQUENCY CONFIG_I2C_MASTER_FREQUENCY
#define PORT_NUMBER -1

static int32_t bmp280RawPressure = 0;
static int32_t bmp280RawTemperature = 0;

#define BMP280_PRESSURE_OSR			(BMP280_OVERSAMP_8X)
#define BMP280_TEMPERATURE_OSR		(BMP280_OVERSAMP_16X)
#define BMP280_MODE					(BMP280_PRESSURE_OSR << 2 | BMP280_TEMPERATURE_OSR << 5 | BMP280_NORMAL_MODE)

bmp280Calib  bmp280Cal;
i2c_device_handle_t bmp280_handle;

static void BMP280GetPressure(void)
{
    uint8_t data[BMP280_DATA_FRAME_SIZE];

    // read data from sensor
    ESP_ERROR_CHECK(i2c_device_read(bmp280_handle, BMP280_PRESSURE_MSB_REG, (uint8_t *)&data, BMP280_DATA_FRAME_SIZE));
    bmp280RawPressure = (int32_t)((((uint32_t)(data[0])) << 12) | (((uint32_t)(data[1])) << 4) | ((uint32_t)data[2] >> 4));
    bmp280RawTemperature = (int32_t)((((uint32_t)(data[3])) << 12) | (((uint32_t)(data[4])) << 4) | ((uint32_t)data[5] >> 4));
}

// Returns temperature in DegC, resolution is 0.01 DegC. Output value of "5123" equals 51.23 DegC
// t_fine carries fine temperature as global value
static int32_t BMP280CompensateT(int32_t adcT)
{
    int32_t var1, var2, T;

    var1 = ((((adcT >> 3) - ((int32_t)bmp280Cal.dig_T1 << 1))) * ((int32_t)bmp280Cal.dig_T2)) >> 11;
    var2  = (((((adcT >> 4) - ((int32_t)bmp280Cal.dig_T1)) * ((adcT >> 4) - ((int32_t)bmp280Cal.dig_T1))) >> 12) * ((int32_t)bmp280Cal.dig_T3)) >> 14;
    bmp280Cal.t_fine = var1 + var2;

    T = (bmp280Cal.t_fine * 5 + 128) >> 8;

    return T;
}

// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of "24674867" represents 24674867/256 = 96386.2 Pa = 963.862 hPa
static uint32_t BMP280CompensateP(int32_t adcP)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)bmp280Cal.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)bmp280Cal.dig_P6;
    var2 = var2 + ((var1*(int64_t)bmp280Cal.dig_P5) << 17);
    var2 = var2 + (((int64_t)bmp280Cal.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)bmp280Cal.dig_P3) >> 8) + ((var1 * (int64_t)bmp280Cal.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)bmp280Cal.dig_P1) >> 33;
    if (var1 == 0)
        return 0;
    p = 1048576 - adcP;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)bmp280Cal.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)bmp280Cal.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)bmp280Cal.dig_P7) << 4);
    return (uint32_t)p;
}

#define CONST_PF 0.1902630958	//(1/5.25588f) Pressure factor
#define FIX_TEMP 25				// Fixed Temperature. ASL is a function of pressure and temperature, but as the temperature changes so much (blow a little towards the flie and watch it drop 5 degrees) it corrupts the ASL estimates.
								// TLDR: Adjusting for temp changes does more harm than good.
/**
 * Converts pressure to altitude above sea level (ASL) in meters
 */
static float BMP280PressureToAltitude(float* pressure/*, float* groundPressure, float* groundTemp*/)
{
    if (*pressure > 0)
    {
        return ((pow((1015.7f / *pressure), CONST_PF) - 1.0f) * (FIX_TEMP + 273.15f)) / 0.0065f;
    }
    else
    {
        return 0;
    }
}

#define FILTER_NUM	5
#define FILTER_A	0.1f

/* 限副平均滤波法 */
static void presssureFilter(float* in, float* out)
{
	static uint8_t i = 0;
	static float filter_buf[FILTER_NUM] = {0.0};
	double filter_sum = 0.0;
	uint8_t cnt = 0;
	float deta;

	if(filter_buf[i] == 0.0f)
	{
		filter_buf[i] = *in;
		*out = *in;
		if(++i >= FILTER_NUM)
            i=0;
	}
    else
	{
		if(i)
            deta = *in-filter_buf[i - 1];
		else
            deta = *in-filter_buf[FILTER_NUM - 1];

		if(fabs(deta) < FILTER_A)
		{
			filter_buf[i] = *in;
			if(++i >= FILTER_NUM)
                i = 0;
		}
		for(cnt = 0; cnt < FILTER_NUM; cnt++)
		{
			filter_sum += filter_buf[cnt];
		}
		*out = filter_sum / FILTER_NUM;
	}
}

void BMP280GetData(float* pressure, float* temperature, float* asl)
{
    static float t;
    static float p;

	BMP280GetPressure();
	t = BMP280CompensateT(bmp280RawTemperature) / 100.0;
	p = BMP280CompensateP(bmp280RawPressure) / 25600.0;
	presssureFilter(&p, pressure);
	*temperature = (float)t;/*单位摄氏度*/

	*asl = BMP280PressureToAltitude(pressure);	/* 转换成海拔 */
}

uint8_t bmp280_init(void)
{
    uint8_t bmp280_id;
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

    i2c_sensor_config_t bmp280_config = {
        .device.scl_speed_hz = MASTER_FREQUENCY,
        .device.device_address = BMP280_SLAVE_ADDRESS,
        .addr_wordlen = 1,
        .write_time_ms = 10,
    };

    ESP_ERROR_CHECK(i2c_device_init(bus_handle, &bmp280_config, &bmp280_handle));

    /* read calibration data */
    ESP_ERROR_CHECK(i2c_device_read(bmp280_handle, BMP280_CHIPID_REG, &bmp280_id, 1));
    ESP_ERROR_CHECK(i2c_device_read(bmp280_handle, BMP280_DIG_T1_LSB_REG, (uint8_t *)&bmp280Cal, 24));
    tmp[0] = BMP280_MODE;
    ESP_ERROR_CHECK(i2c_device_write(bmp280_handle, BMP280_CTRLMEAS_REG, tmp, 1));
    tmp[0] = (5<<2);
    ESP_ERROR_CHECK(i2c_device_write(bmp280_handle, BMP280_CONFIG_REG, tmp, 1));
    return bmp280_id;
}