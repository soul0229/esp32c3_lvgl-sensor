/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once
#include <stdio.h>

#define BH1750_SLAVE_ADDRESS    0x23		/* I2C从机地址 */

//****************************************
// 定义 ATH20 内部地址
//****************************************
#define	POWER_DOWN		    0X00	//初始化
#define	POWER_ON		    0X01	//初始化
#define	RESET		        0X07	//初始化
#define	HRES_MODE		    0X10	//1LX 120MS
#define	HRES_MODE2		    0X11    //0.5LX 120MS
#define	LRES_MODE		    0X13    //4LX 16MS
#define	ONE_HRES_MODE		0X20    //1LX 120MS,AUTO POWER DOWN
#define	ONE_HRES_MODE2		0X21    //0.5LX 120MS,AUTO POWER DOWN
#define	ONE_LRES_MODE		0X23    //4LX 16MS,AUTO POWER DOWN
#define	CM_TIMEH		    0X40    //HIGHT BIT ----_-[7,6,5]
#define	CM_TIMEL		    0X60    //LOW BIT ---[4]_[3,2,1,0]

uint8_t BH1750_Init(void);
float Multiple_Read_BH1750();
