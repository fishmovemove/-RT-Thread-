/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-15     fish move move       the first version
 */

#include "tub.h"
#include <rtdevice.h>
#include <rtthread.h>

#define ADC_DEV_NAME        "adc1"      // ADC 设备名称
#define ADC_CHANNEL         3            // 可根据硬件选择通道编号
#define TS_READ_TIMES       10
#define TS_K                2047.19      // 自定义校准值

static rt_adc_device_t adc_dev = RT_NULL;

void turb_adc_init(void)
{
    adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("Can't find %s device!\n", ADC_DEV_NAME);
        return;
    }

    rt_adc_enable(adc_dev, ADC_CHANNEL);
}

uint16_t turb_adc_read_raw(void)
{
    return rt_adc_read(adc_dev, ADC_CHANNEL);
}

float turb_get_data(void)
{
    float tempRaw = 0;
    for (uint8_t i = 0; i < TS_READ_TIMES; i++)
    {
        tempRaw += turb_adc_read_raw();
        rt_thread_mdelay(5);
    }
    tempRaw /= TS_READ_TIMES;

    float voltage = (tempRaw/4096.0)*3.3;
    float ntu = -865.68*voltage+TS_K;    // 简化线性映射：1.5V ~ 0 NTU，2.5V ~ 3000 NTU
    if (ntu < 0)
        ntu = 0;
    return ntu;
}

