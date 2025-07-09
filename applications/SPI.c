/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-29     fish move move       the first version
 */
#include "lcd.h"
#include "ili9341.h"
#include "SPI.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

static int filter_on = 0;
static int light_on  = 0;
static int heater_on = 0;
static int feed_on   = 0;

#define base_x 10
#define base_y 30
#define line_height 20
#define ui_base_x 150

#define SPI_CS_PIN   GET_PIN(F, 5)
#define SPI_CLK_PIN  GET_PIN(F, 6)
#define SPI_MOSI_PIN GET_PIN(B, 11)
#define SPI_MISO_PIN GET_PIN(B, 10)
#define SPI_IRQ_PIN  GET_PIN(F, 4)

#define WHITE 0xFFFF
#define BLACK 0x0000
#define GREEN  0x07E0  // 绿色，RGB565格式
#define RED    0xF800  // 红色，RGB565格式
#define ILI9341_WIDTH  240

static void spi_delay(void)
{
    for (volatile int i = 0; i < 10; i++);
}

void soft_spi_touch_init(void)
{
    rt_pin_mode(SPI_CS_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(SPI_CLK_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(SPI_MOSI_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(SPI_MISO_PIN, PIN_MODE_INPUT);
    rt_pin_mode(SPI_IRQ_PIN, PIN_MODE_INPUT);

    rt_pin_write(SPI_CS_PIN, PIN_HIGH);
    rt_pin_write(SPI_CLK_PIN, PIN_LOW);
}

static uint8_t soft_spi_transfer(uint8_t data)
{
    uint8_t recv = 0;
    for (int i = 0; i < 8; i++)
    {
        rt_pin_write(SPI_MOSI_PIN, (data & 0x80) ? PIN_HIGH : PIN_LOW);
        data <<= 1;

        rt_pin_write(SPI_CLK_PIN, PIN_HIGH);
        spi_delay();

        recv <<= 1;
        if (rt_pin_read(SPI_MISO_PIN))
            recv |= 1;

        rt_pin_write(SPI_CLK_PIN, PIN_LOW);
        spi_delay();
    }
    return recv;
}

static uint16_t xpt2046_read_ad(uint8_t cmd)
{
    uint8_t h, l;
    uint16_t val;

    rt_pin_write(SPI_CS_PIN, PIN_LOW);
    soft_spi_transfer(cmd);
    h = soft_spi_transfer(0x00);
    l = soft_spi_transfer(0x00);
    rt_pin_write(SPI_CS_PIN, PIN_HIGH);

    val = ((h << 8) | l) >> 3;
    return val;
}

rt_bool_t xpt2046_get_touch(uint16_t *x, uint16_t *y)
{
    if (rt_pin_read(SPI_IRQ_PIN) == PIN_LOW)
    {
        rt_kprintf("[touch] IRQ triggered, trying to read...\n");

        *x = xpt2046_read_ad(0xD0); // X 通道
        *y = xpt2046_read_ad(0x90); // Y 通道

        rt_kprintf("[touch] Raw ADC: x=%d, y=%d\n", *x, *y);
        return RT_TRUE;
    }

    return RT_FALSE;
}


