/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-28     fish move move       the first version
 */
#include "ili9341.h"
#include <rtdevice.h>
#include <board.h>
#include "drv_gpio.h"
#include "drv_spi.h"
#include <rtdbg.h>

#define DBG_TAG "ili9341"
#define DBG_LVL DBG_LOG

#define PIN_CS   GET_PIN(A, 4)
#define PIN_DC   GET_PIN(E, 3)
#define PIN_RST  GET_PIN(E, 4)

static struct rt_spi_device *spi_dev = RT_NULL;
uint16_t lcd_width = 320, lcd_height = 240;

void ili9341_write_byte(rt_uint8_t data, rt_bool_t is_cmd)
{
    rt_pin_write(PIN_DC, is_cmd ? 0 : 1);
    rt_pin_write(PIN_CS, 0);
    rt_spi_transfer(spi_dev, &data, RT_NULL, 1);
    rt_pin_write(PIN_CS, 1);
}

void ili9341_set_window(rt_uint16_t x0, rt_uint16_t y0, rt_uint16_t x1, rt_uint16_t y1)
{
    ili9341_write_byte(0x2A, RT_TRUE);
    ili9341_write_byte(x0 >> 8, RT_FALSE);
    ili9341_write_byte(x0 & 0xFF, RT_FALSE);
    ili9341_write_byte(x1 >> 8, RT_FALSE);
    ili9341_write_byte(x1 & 0xFF, RT_FALSE);

    ili9341_write_byte(0x2B, RT_TRUE);
    ili9341_write_byte(y0 >> 8, RT_FALSE);
    ili9341_write_byte(y0 & 0xFF, RT_FALSE);
    ili9341_write_byte(y1 >> 8, RT_FALSE);
    ili9341_write_byte(y1 & 0xFF, RT_FALSE);

    ili9341_write_byte(0x2C, RT_TRUE);
}

void ili9341_hw_spi_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    rt_kprintf("[I/ili9341] SPI GPIO AF Init OK\n");
}

void ili9341_gpio_init(void)
{
    rt_pin_mode(PIN_CS,  PIN_MODE_OUTPUT);
    rt_pin_mode(PIN_DC,  PIN_MODE_OUTPUT);
    rt_pin_mode(PIN_RST, PIN_MODE_OUTPUT);

    rt_pin_write(PIN_CS, 1);
    rt_pin_write(PIN_DC, 1);
    rt_pin_write(PIN_RST, 1);

    rt_pin_write(PIN_RST, 0);
    rt_thread_mdelay(50);
    rt_pin_write(PIN_RST, 1);
    rt_thread_mdelay(120);

    rt_kprintf("[I/ili9341] GPIO Init OK\n");
}

void ili9341_set_rotation(uint8_t m)
{
    ili9341_write_byte(0x36, RT_TRUE);
    switch (m)
    {
        case 0: ili9341_write_byte(0x48, RT_FALSE); lcd_width = 240; lcd_height = 320; break;
        case 1: ili9341_write_byte(0x28, RT_FALSE); lcd_width = 320; lcd_height = 240; break;
        case 2: ili9341_write_byte(0x88, RT_FALSE); lcd_width = 240; lcd_height = 320; break;
        case 3: ili9341_write_byte(0xE8, RT_FALSE); lcd_width = 320; lcd_height = 240; break;
    }
}

void ili9341_init(void)
{
    spi_dev = (struct rt_spi_device *)rt_device_find("spi10");
    if (!spi_dev)
    {
        rt_kprintf("[E/ili9341] spi device spi10 not found!\n");
        return;
    }

    ili9341_write_byte(0x01, RT_TRUE);
    rt_thread_mdelay(100);
    ili9341_write_byte(0x28, RT_TRUE);
    ili9341_write_byte(0x3A, RT_TRUE);
    ili9341_write_byte(0x55, RT_FALSE);

    ili9341_set_rotation(1); // 横屏

    ili9341_write_byte(0x11, RT_TRUE);
    rt_thread_mdelay(120);
    ili9341_write_byte(0x29, RT_TRUE);

    rt_kprintf("[I/ili9341] Initialized\n");
}

void ili9341_fill_color(rt_uint16_t color)
{
    ili9341_set_window(0, 0, lcd_width - 1, lcd_height - 1);

    rt_uint8_t data[2] = {color >> 8, color & 0xFF};
    for (int i = 0; i < lcd_width * lcd_height; i++)
    {
        ili9341_write_byte(data[0], RT_FALSE);
        ili9341_write_byte(data[1], RT_FALSE);
    }

    rt_kprintf("[I/ili9341] Screen filled with color: 0x%04X\n", color);
}

void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    ili9341_set_window(x, y, x, y);
    ili9341_write_byte(color >> 8, RT_FALSE);
    ili9341_write_byte(color & 0xFF, RT_FALSE);
}
