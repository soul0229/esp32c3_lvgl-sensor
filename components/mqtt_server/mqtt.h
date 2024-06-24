/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */


#pragma once
#include <stdio.h>
typedef struct msg_queen {
#define MSQ_QUEEN_NUM 2
#define MSQ_QUEEN_LEN 64
int tempture[MSQ_QUEEN_NUM][MSQ_QUEEN_LEN];
int humidity[MSQ_QUEEN_NUM][MSQ_QUEEN_LEN];
int barometer[MSQ_QUEEN_NUM][MSQ_QUEEN_LEN];
int brightness[MSQ_QUEEN_NUM][MSQ_QUEEN_LEN];
unsigned int cnt[MSQ_QUEEN_NUM];
}msg_queen_t;

void mqtt_main(void);
void my_mqtt_start(void);
void my_mqtt_stop(void);

