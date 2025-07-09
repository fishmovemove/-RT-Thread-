/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-28     fish move move       the first version
 */
#include "lcd.h"
#include "lcd_font.h"
#include "ili9341.h"

#include "lcd.h"
#include "lcd_font.h"     // 这里假设包含了 font8x8_basic 定义
#include "ili9341.h"

extern char font8x8_basic[128][8]; // 声明外部字库
extern uint16_t lcd_width;
extern uint16_t lcd_height;
/*
void lcd_draw_char(uint16_t x, uint16_t y, char chr, uint16_t color, uint16_t bgcolor)
{
    if (chr < 0 || chr > 127) chr = '?';

    for (uint8_t row = 0; row < 8; row++)
    {
        uint8_t line = font8x8_basic[(uint8_t)chr][row];
        for (uint8_t col = 0; col < 8; col++)
        {
            if (line & (1 << col))  // 原来是 (0x80 >> col)
                lcd_draw_pixel(x + col, y + row, color);
            else
                lcd_draw_pixel(x + col, y + row, bgcolor);
        }
    }
}

void lcd_draw_string(uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bgcolor)
{
    while (*str)
    {
        lcd_draw_char(x, y, *str, color, bgcolor);
        x += 8;
        if (x + 8 >= 240) // 横向边界换行（适配你的3.2寸屏幕，分辨率240x320）
        {
            x = 0;
            y += 8;       // 注意：字符高度是8像素（不是原来的16）
        }
        str++;
    }
}

void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    ili9341_set_window(x, y, x + w - 1, y + h - 1);
    rt_uint8_t data[2] = {color >> 8, color & 0xFF};
    for (int i = 0; i < w * h; i++)
    {
        ili9341_write_byte(data[0], RT_FALSE);
        ili9341_write_byte(data[1], RT_FALSE);
    }
}
*/
void lcd_draw_char(uint16_t x, uint16_t y, char chr, uint16_t color, uint16_t bgcolor)
{
    if (chr < 0 || chr > 127) chr = '?';

    for (uint8_t row = 0; row < 8; row++)
    {
        uint8_t line = font8x8_basic[(uint8_t)chr][row];
        for (uint8_t col = 0; col < 8; col++)
        {
            if (line & (1 << col))
                lcd_draw_pixel(x + col, y + row, color);
            else
                lcd_draw_pixel(x + col, y + row, bgcolor);
        }
    }
}

void lcd_draw_string(uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bgcolor)
{
    while (*str)
    {
        lcd_draw_char(x, y, *str, color, bgcolor);
        x += 8;

        // 【修改】改为动态屏幕宽度判断换行，避免写死240导致坐标错乱
        if (x + 8 >= lcd_width)
        {
            x = 0;
            y += 8;
        }
        str++;
    }
}

void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    // 防止越界，自动修正宽高
    if (x + w > lcd_width)
        w = lcd_width - x;
    if (y + h > lcd_height)
        h = lcd_height - y;

    ili9341_set_window(x, y, x + w - 1, y + h - 1);

    rt_uint8_t data[2] = {color >> 8, color & 0xFF};
    for (int i = 0; i < w * h; i++)
    {
        ili9341_write_byte(data[0], RT_FALSE);
        ili9341_write_byte(data[1], RT_FALSE);
    }
}




