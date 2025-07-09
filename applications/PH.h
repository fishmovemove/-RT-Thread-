/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-15     fish move move       the first version
 */
#ifndef APPLICATIONS_PH_H_
#define APPLICATIONS_PH_H_

typedef struct
{
    float ph4_voltage;    // 4点校准电压
    float ph7_voltage;    // 7点校准电压
    float ph9_voltage;    // 9点校准电压
    float slope;          // 斜率
    float intercept;      // 截距
} PH_Calibration;

void ph_sensor_init(void);
float ph_sensor_get_voltage(void);
float ph_sensor_get_value(void);
void ph_sensor_calibrate(unsigned char point, float known_ph);
void ph_sensor_update_calibration(void);
PH_Calibration *ph_sensor_get_calib(void);

// 存储和读取校准数据接口（根据你的存储需求实现）
void ph_calib_data_load(PH_Calibration *calib);
void ph_calib_data_save(PH_Calibration *calib);

#endif /* APPLICATIONS_PH_H_ */
