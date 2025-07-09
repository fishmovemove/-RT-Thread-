/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-15     fish move move       the first version
 */
#ifndef APPLICATIONS_TDS_H_
#define APPLICATIONS_TDS_H_

#include <rtthread.h>
#include <rtdevice.h>
#include <stdint.h>

void TDS_Init(void);
uint16_t TDS_GetData(void);
float TDS_GetData_PPM(void);

#endif /* APPLICATIONS_TDS_H_ */
