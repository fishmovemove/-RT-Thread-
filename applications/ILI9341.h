/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-28     fish move move       the first version
 */
#ifndef APPLICATIONS_ILI9341_H_
#define APPLICATIONS_ILI9341_H_

#include <rtthread.h>

void ili9341_write_byte(rt_uint8_t data, rt_bool_t is_cmd);
void ili9341_set_window(rt_uint16_t x0, rt_uint16_t y0, rt_uint16_t x1, rt_uint16_t y1);
void ili9341_hw_spi_init(void);
void ili9341_gpio_init(void);
void ili9341_init(void);
void ili9341_fill_color(rt_uint16_t color);
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

#endif /* APPLICATIONS_ILI9341_H_ */
