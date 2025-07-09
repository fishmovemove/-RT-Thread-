/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-15     fish move move       the first version
 */
#ifndef APPLICATIONS_DS18B20_H_
#define APPLICATIONS_DS18B20_H_

uint8_t DS18B20_Init(void);
void DS18B20_Start(void);
short DS18B20_Get_Temp(void);
uint8_t DS18B20_Read_Byte(void);
void DS18B20_Write_Byte(uint8_t dat);
uint8_t DS18B20_Read_Bit(void);
uint8_t DS18B20_Check(void);
void DS18B20_Rst(void);

#endif /* APPLICATIONS_DS18B20_H_ */
