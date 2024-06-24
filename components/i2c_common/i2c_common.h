/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once

#include <stdint.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    i2c_device_config_t device;  /*!< Configuration for device device */
    uint8_t addr_wordlen;               /*!< block address wordlen */
    uint8_t write_time_ms;              /*!< device write time, typically 10ms*/
} i2c_sensor_config_t;

struct i2c_device_t {
    i2c_master_dev_handle_t i2c_dev;      /*!< I2C device handle */
    uint8_t addr_wordlen;                 /*!< block address wordlen */
    uint8_t *buffer;                      /*!< I2C transaction buffer */
    uint8_t write_time_ms;                /*!< I2C device write time(ms)*/
};

typedef struct i2c_device_t i2c_device_t;

/* handle of I2C device */
typedef struct i2c_device_t *i2c_device_handle_t;

/**
 * @brief Init an I2C device.
 *
 * @param[in] bus_handle I2C master bus handle
 * @param[in] device_config Configuration of I2C
 * @param[out] device_handle Handle of I2C
 * @return ESP_OK: Init success. ESP_FAIL: Not success.
 */
esp_err_t i2c_device_init(i2c_master_bus_handle_t bus_handle, const i2c_sensor_config_t *device_config, i2c_device_handle_t *device_handle);

/**
 * @brief Write data to I2C
 *
 * @param[in] device_handle I2C handle
 * @param[in] address Block address inside I2C
 * @param[in] data Data to write
 * @param[in] size Data write size
 * @return ESP_OK: Write success. Otherwise failed, please check I2C function fail reason.
 */
esp_err_t i2c_device_write(i2c_device_handle_t device_handle, uint32_t address, const uint8_t *data, uint32_t size);

/**
 * @brief Read data from I2C
 *
 * @param device_handle I2C handle
 * @param address Block address inside I2C
 * @param data Data read from I2C
 * @param size Data read size
 * @return ESP_OK: Read success. Otherwise failed, please check I2C function fail reason.
 */
esp_err_t i2c_device_read(i2c_device_handle_t device_handle, uint32_t address, uint8_t *data, uint32_t size);

/**
 * @brief Wait device finish. Typically 5ms
 *
 * @param device_handle I2C handle
 */
void i2c_device_wait_idle(i2c_device_handle_t device_handle);

#ifdef __cplusplus
}
#endif
