/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-15     fish move move       the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "ds18b20.h"

#define DS18B20_PIN    GET_PIN(A, 8)

static void DS18B20_Mode(rt_base_t mode)
{
    rt_pin_mode(DS18B20_PIN, mode);
}

static void DS18B20_Write_Pin(rt_base_t value)
{
    rt_pin_write(DS18B20_PIN, value);
}

static rt_base_t DS18B20_Read_Pin(void)
{
    return rt_pin_read(DS18B20_PIN);
}

void DS18B20_Rst(void)
{
    DS18B20_Mode(PIN_MODE_OUTPUT);
    DS18B20_Write_Pin(0);
    rt_hw_us_delay(750);
    DS18B20_Write_Pin(1);
    rt_hw_us_delay(15);
}

uint8_t DS18B20_Check(void)
{
    uint8_t retry = 0;
    DS18B20_Mode(PIN_MODE_INPUT);

    while (DS18B20_Read_Pin() && retry < 200)
    {
        retry++;
        rt_hw_us_delay(1);
    }

    if (retry >= 200)
        return 1;

    retry = 0;
    while (!DS18B20_Read_Pin() && retry < 240)
    {
        retry++;
        rt_hw_us_delay(1);
    }

    return (retry >= 240) ? 1 : 0;
}

uint8_t DS18B20_Read_Bit(void)
{
    uint8_t data;
    DS18B20_Mode(PIN_MODE_OUTPUT);
    DS18B20_Write_Pin(0);
    rt_hw_us_delay(2);
    DS18B20_Write_Pin(1);
    DS18B20_Mode(PIN_MODE_INPUT);
    rt_hw_us_delay(12);

    data = DS18B20_Read_Pin();
    rt_hw_us_delay(50);
    return data;
}

uint8_t DS18B20_Read_Byte(void)
{
    uint8_t i, j, dat = 0;

    for (i = 1; i <= 8; i++)
    {
        j = DS18B20_Read_Bit();
        dat = (j << 7) | (dat >> 1);
    }
    return dat;
}

void DS18B20_Write_Byte(uint8_t dat)
{
    uint8_t j, testb;

    for (j = 1; j <= 8; j++)
    {
        testb = dat & 0x01;
        dat >>= 1;

        DS18B20_Mode(PIN_MODE_OUTPUT);
        if (testb)
        {
            DS18B20_Write_Pin(0);
            rt_hw_us_delay(2);
            DS18B20_Write_Pin(1);
            rt_hw_us_delay(60);
        }
        else
        {
            DS18B20_Write_Pin(0);
            rt_hw_us_delay(60);
            DS18B20_Write_Pin(1);
            rt_hw_us_delay(2);
        }
    }
}

void DS18B20_Start(void)
{
    DS18B20_Rst();
    DS18B20_Check();
    DS18B20_Write_Byte(0xCC);  // Skip ROM
    DS18B20_Write_Byte(0x44);  // Convert T
}

uint8_t DS18B20_Init(void)
{
    rt_pin_mode(DS18B20_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(DS18B20_PIN, 1);
    DS18B20_Rst();
    return DS18B20_Check();
}

short DS18B20_Get_Temp(void)
{
    uint8_t TL, TH;
    uint8_t sign = 1;
    short temp;

    DS18B20_Start();
    DS18B20_Rst();
    DS18B20_Check();
    DS18B20_Write_Byte(0xCC);  // Skip ROM
    DS18B20_Write_Byte(0xBE);  // Read Scratchpad

    TL = DS18B20_Read_Byte();
    TH = DS18B20_Read_Byte();

    if (TH > 7)
    {
        TH = ~TH;
        TL = ~TL;
        sign = 0;
    }

    temp = (TH << 8) | TL;
    temp = (float)temp * 0.625;

    return sign ? temp : -temp;
}

