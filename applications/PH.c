/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-15     fish move move       the first version
 */

#include "ph.h"
#include <drivers/adc.h>
#include <math.h>
#include <string.h>

#define PH_ADC_DEV_NAME    "adc1"
#define PH_ADC_CHANNEL     2
#define VREF               3.3f
#define ADC_RESOLUTION     4096.0f
#define MODULE_VCC         5.0f

static rt_adc_device_t adc_dev = RT_NULL;
static PH_Calibration ph_calib;

void ph_sensor_init(void)
{
    adc_dev = (rt_adc_device_t)rt_device_find(PH_ADC_DEV_NAME);
    if (adc_dev)
    {
        rt_adc_enable(adc_dev, PH_ADC_CHANNEL);
    }
    else
    {
        rt_kprintf("ADC device %s not found!\n", PH_ADC_DEV_NAME);
    }

    // 初始化校准参数，防止全0
       ph_calib.slope = -0.057f;     // 这里填你默认的斜率
       ph_calib.intercept = 7.0f;    // 这里填你默认的截距

    ph_calib_data_load(&ph_calib);
}

float ph_sensor_get_voltage(void)
{
    if (!adc_dev) return 0.0f;
    int value = rt_adc_read(adc_dev, PH_ADC_CHANNEL);
    float adc_v = (value * VREF) / ADC_RESOLUTION;
    float module_v = adc_v * (MODULE_VCC / VREF);
    if (module_v > 5.0f) module_v = 5.0f;
    if (module_v < 0.0f) module_v = 0.0f;
    return module_v;
}

float ph_sensor_get_value(void)
{
    float v = ph_sensor_get_voltage();
    return ph_calib.slope * v + ph_calib.intercept;
}

void ph_sensor_calibrate(unsigned char point, float known_ph)
{
    float v_sum = 0;
    for (int i = 0; i < 10; i++) {
        v_sum += ph_sensor_get_voltage();
        rt_thread_mdelay(100);
    }
    float v_avg = v_sum / 10.0f;

    if (point == 4) ph_calib.ph4_voltage = v_avg;
    else if (point == 7) ph_calib.ph7_voltage = v_avg;
    else if (point == 9) ph_calib.ph9_voltage = v_avg;

    ph_sensor_update_calibration();
    ph_calib_data_save(&ph_calib);
}

void ph_sensor_update_calibration(void)
{
    if (ph_calib.ph4_voltage > 0 && ph_calib.ph9_voltage > 0 && ph_calib.ph7_voltage > 0)
    {
        float slope1 = (7.00f - 4.00f) / (ph_calib.ph7_voltage - ph_calib.ph4_voltage);
        float slope2 = (9.18f - 7.00f) / (ph_calib.ph9_voltage - ph_calib.ph7_voltage);
        ph_calib.slope = (slope1 + slope2) / 2.0f;
    }
    else
    {
        ph_calib.slope = -0.057f;  // 默认斜率
    }
    ph_calib.intercept = 7.00f;
}

PH_Calibration *ph_sensor_get_calib(void)
{
    return &ph_calib;
}

// 这里给出空实现，方便你根据需要自行实现数据保存和加载
void ph_calib_data_load(PH_Calibration *calib)
{
    //memset(calib, 0, sizeof(PH_Calibration));
    // TODO: 从flash或其他介质读取校准数据
}

void ph_calib_data_save(PH_Calibration *calib)
{
    // TODO: 将校准数据保存到flash或其他介质
}

