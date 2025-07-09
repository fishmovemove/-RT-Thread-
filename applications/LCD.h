/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-28     fish move move       the first version
 */
#ifndef APPLICATIONS_LCD_H_
#define APPLICATIONS_LCD_H_


#include <rtthread.h>
#include <rtdevice.h>

void lcd_draw_char(rt_uint16_t x, rt_uint16_t y, char ch, rt_uint16_t color, rt_uint16_t bgcolor);
void lcd_draw_string(rt_uint16_t x, rt_uint16_t y, const char *str, rt_uint16_t color, rt_uint16_t bgcolor);
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
//void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);


#endif /* APPLICATIONS_LCD_H_ */

