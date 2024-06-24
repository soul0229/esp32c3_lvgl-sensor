/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once
#include <stdio.h>

#define ATH20_SLAVE_ADDRESS    0x38		/* I2C从机地址 */

//****************************************
// 定义 ATH20 内部地址
//****************************************
#define	INIT		    0xBE	//初始化
#define	SoftReset		0xBA	//软复位
#define	StartTest		0xAC	//开始测试

uint8_t ATH20_Init(void);
uint8_t ATH20_Read_Cal_Enable(void);  //查询cal enable位有没有使能
void ATH20_Read_CTdata(uint32_t *ct); //读取AHT20的温度和湿度
