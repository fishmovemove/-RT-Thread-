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
#include "tds.h"
#include <drivers/adc.h>
#define TDS_ADC_DEV_NAME     "adc1"         // ADC设备名
#define TDS_ADC_CHANNEL      1            // ADC通道号（例如PA1）
#define TDS_READ_TIMES       10             // 采样次数

static rt_adc_device_t adc_dev = RT_NULL;

void TDS_Init(void)
{
    adc_dev = (rt_adc_device_t)rt_device_find(TDS_ADC_DEV_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("TDS: cannot find %s\n", TDS_ADC_DEV_NAME);
        return;
    }

    rt_adc_enable(adc_dev, TDS_ADC_CHANNEL);
    rt_kprintf("TDS: ADC device initialized.\n");
}

INIT_APP_EXPORT(TDS_Init);



static uint16_t TDS_ADC_Read(void)
{
    if (adc_dev == RT_NULL)
        return 0;

    return rt_adc_read(adc_dev, TDS_ADC_CHANNEL);
}

uint16_t TDS_GetData(void)
{
    uint32_t sum = 0;
    for (int i = 0; i < TDS_READ_TIMES; i++)
    {
        sum += TDS_ADC_Read();
        rt_thread_mdelay(5);  // 延时防止采样过快
    }
    return sum / TDS_READ_TIMES;
}


float TDS_GetData_PPM(void)
{
    float adc_avg = 0.0f;

    for (int i = 0; i < TDS_READ_TIMES; i++)
    {
        adc_avg += TDS_ADC_Read();
        rt_thread_mdelay(5);
    }
    adc_avg /= TDS_READ_TIMES;

    // 将 ADC 原始数据转换为电压（假设参考电压为 3.3V，12位精度）
    float voltage = (adc_avg / 4095.0f) * 3.3f;

    // 使用经验公式计算 TDS ppm 值
    float ppm = 66.71f * voltage * voltage * voltage
              - 127.93f * voltage * voltage
              + 428.7f * voltage;
    return ppm;
    //return voltage;
}

