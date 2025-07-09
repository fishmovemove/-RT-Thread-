/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-5-10      ShiHao       first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "paho_mqtt.h"
#include <wlan_mgnt.h>

#include "TDS.h"
#include "PH.h"
#include "TUB.h"
#include "DS18B20.h"
#include "lcd.h"
#include "ili9341.h"
#include "SPI.h"
#include <drv_spi.h>
#include <drv_gpio.h>

#define SPI_CS_PIN   GET_PIN(F, 5)
#define SPI_CLK_PIN  GET_PIN(F, 6)
#define SPI_MOSI_PIN GET_PIN(B, 11)
#define SPI_MISO_PIN GET_PIN(B, 10)
#define SPI_IRQ_PIN  GET_PIN(F, 4)

#define WHITE 0xFFFF
#define BLACK 0x0000
#define GREEN  0x07E0  // 绿色，RGB565格式
#define RED    0xF800  // 红色，RGB565格式


#define LIGHT GET_PIN(D,9)
#define FILTER GET_PIN(D,8)
#define FEED GET_PIN(D,10)
#define FEED_DETECT_PIN   GET_PIN(D, 12)
#define FILTER_DETECT_PIN GET_PIN(D,11)
#define LIGHT_DETECT_PIN GET_PIN(F,7)

#define WHITE 0xFFFF
#define BLACK 0x0000
#define ILI9341_WIDTH  240

volatile int relay_feed_state   = 0;
volatile int relay_filter_state = 0;
volatile int relay_light_state  = 0;


static void spi1_init(void)
{
    ili9341_hw_spi_init();
    rt_hw_spi_device_attach("spi1", "spi10", GPIOA, GPIO_PIN_4);
    struct rt_spi_device *spi_dev = (struct rt_spi_device *)rt_device_find("spi10");
    if (!spi_dev)
    {
        rt_kprintf("[E/main] spi10 not found!\n");
        return;
    }

    struct rt_spi_configuration cfg = {
        .mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB,
        .data_width = 8,
        .max_hz = 10 * 1000 * 1000,
    };
    rt_spi_configure(spi_dev, &cfg);

    rt_kprintf("[I/main] SPI Initialized\n");
}

extern void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);


static void sensor_display_thread(void *parameter)
{
    TDS_Init();
    DS18B20_Init();
    ph_sensor_init();
    turb_adc_init();

    float last_temp = -100.0f;
    float last_ph   = -1.0f;
    float last_tds  = -1.0f;
    float last_turb = -1.0f;

    char temp_buf[16] = {0};
    char ph_buf[16]   = {0};
    char tds_buf[16]  = {0};
    char turb_buf[16] = {0};

    const int base_x = 10;
    const int base_y = 30;
    const int line_height = 48;

    while (1)
    {
        int temp_raw = DS18B20_Get_Temp();
        float ph   = ph_sensor_get_value();
        float tds  = TDS_GetData_PPM();
        float turb = turb_get_data();

        if ((int)(temp_raw * 10) != (int)(last_temp * 10))
        {
            int ip = temp_raw / 10;
            int fp = temp_raw % 10;
            rt_snprintf(temp_buf, sizeof(temp_buf), "%d.%d C", ip, fp);

            lcd_draw_string(base_x, base_y + line_height * 0, "Temp:", BLACK, WHITE);
            lcd_draw_string(base_x + 8 * 6, base_y + line_height * 0, temp_buf, BLACK, WHITE);

            last_temp = temp_raw;
        }

        if ((int)(ph * 100) != (int)(last_ph * 100))
        {
            int ip = (int)ph;
            int fp = (int)(ph * 100) % 100;
            rt_snprintf(ph_buf, sizeof(ph_buf), "%d.%02d", ip, fp);

            lcd_draw_string(base_x, base_y + line_height * 1, "pH  :", BLACK, WHITE);
            lcd_draw_string(base_x + 8 * 6, base_y + line_height * 1, ph_buf, BLACK, WHITE);

            last_ph = ph;
        }

        if ((int)(tds * 10) != (int)(last_tds * 10))
        {
            int ip = (int)tds;
            int fp = (int)(tds * 10) % 10;
            rt_snprintf(tds_buf, sizeof(tds_buf), "%d.%d ppm", ip, fp);

            lcd_draw_string(base_x, base_y + line_height * 2, "TDS :", BLACK, WHITE);
            lcd_draw_string(base_x + 8 * 6, base_y + line_height * 2, tds_buf, BLACK, WHITE);

            last_tds = tds;
        }

        if ((int)(turb * 10) != (int)(last_turb * 10))
        {
            int ip = (int)turb;
            int fp = (int)(turb * 10) % 10;
            rt_snprintf(turb_buf, sizeof(turb_buf), "%d.%d NTU", ip, fp);

            lcd_draw_string(base_x, base_y + line_height * 3, "Turb:", BLACK, WHITE);
            lcd_draw_string(base_x + 8 * 6, base_y + line_height * 3, turb_buf, BLACK, WHITE);

            last_turb = turb;
        }

        rt_thread_mdelay(200);
    }
}


#define base_x 10
#define base_y 30
#define line_height 50
#define ui_base_x 180

static void draw_status(const char *label, int y, int status)
{
    lcd_draw_string(ui_base_x, base_y + line_height * y, label, BLACK, WHITE);
    lcd_draw_string(ui_base_x + 8 * 8, base_y + line_height * y, status ? "ON " : "OFF", status ? GREEN : RED, WHITE);
}

static int filter_on = 0;
static int light_on  = 0;
static int heater_on = 0;
static int feed_on   = 0;

static void update_ui()
{
    draw_status("Filter", 0, filter_on);
    draw_status("Light ", 1, light_on);
    draw_status("Heater", 2, heater_on);
    draw_status("Feed  ", 3, feed_on);
}


/*
static void touch_ui_thread(void *parameter)
{
    uint16_t x, y;
    soft_spi_touch_init();


    // 初始化 GPIO 引脚为输出模式
    rt_pin_mode(LIGHT, PIN_MODE_OUTPUT); // LIGHT
    rt_pin_mode(FILTER, PIN_MODE_OUTPUT);  // FILTER
    rt_pin_mode(FEED, PIN_MODE_OUTPUT); // FEED

    // 初始化状态为关闭
    rt_pin_write(LIGHT, PIN_LOW);  // LIGHT OFF
    rt_pin_write(FILTER, PIN_LOW);   // FILTER OFF
    rt_pin_write(FEED, PIN_LOW);  // FEED OFF

    rt_pin_mode(FEED_DETECT_PIN, PIN_MODE_INPUT);

    if (rt_pin_read(FEED_DETECT_PIN) == PIN_HIGH)
            {
                rt_pin_write(FEED, PIN_HIGH);  // 打开继电器
            }
            else
            {
                rt_pin_write(FEED, PIN_LOW);   // 关闭继电器
            }


    rt_kprintf("[touch_ui] thread started\n");

    rt_bool_t last_pressed = RT_FALSE;

    while (1)
    {
        rt_bool_t pressed = (rt_pin_read(SPI_IRQ_PIN) == PIN_LOW);

        if (pressed && !last_pressed)
        {
            if (xpt2046_get_touch(&x, &y))
            {
                int x_raw_min = 190;
                int x_raw_max = 3853;
                int y_raw_min = 350;
                int y_raw_max = 3979;

                if (x < x_raw_min) x = x_raw_min;
                if (x > x_raw_max) x = x_raw_max;
                if (y < y_raw_min) y = y_raw_min;
                if (y > y_raw_max) y = y_raw_max;

                // 交换XY并翻转，适应横屏显示
                int swapped_x = y;
                int swapped_y = x;

                uint16_t px = 320 - (uint32_t)(swapped_x - y_raw_min) * 320 / (y_raw_max - y_raw_min);
                uint16_t py = 240 - (uint32_t)(swapped_y - x_raw_min) * 240 / (x_raw_max - x_raw_min);

                if (px > 319) px = 319;
                if (py > 239) py = 239;

                // 判断是否点中了控制按钮区域
                int id = (py - base_y) / line_height;
                if (px > ui_base_x + 8 * 8 && px < 320 && id >= 0 && id < 4)
                {
                    switch (id)
                    {
                    case 0:  // Filter
                        filter_on = !filter_on;
                        rt_pin_write(FILTER, filter_on ? PIN_HIGH : PIN_LOW);
                        break;

                    case 1:  // Light
                        light_on = !light_on;
                        rt_pin_write(LIGHT, light_on ? PIN_HIGH : PIN_LOW);
                        break;

                    case 2:  // Heater
                        heater_on = !heater_on;
                        // 暂时不控制实际硬件
                        break;

                    case 3:  // Feed
                        feed_on = !feed_on;
                        rt_pin_write(FEED, feed_on ? PIN_HIGH : PIN_LOW);
                        break;

                    default:
                        break;
                    }

                    update_ui();  // 刷新UI状态
                }
            }
        }

        last_pressed = pressed;
        rt_thread_mdelay(50);
    }
}
*/
/*
static void touch_ui_thread(void *parameter)
{
    uint16_t x, y;
    soft_spi_touch_init();

    // 初始化 GPIO 引脚为输出模式
    rt_pin_mode(LIGHT, PIN_MODE_OUTPUT);
    rt_pin_mode(FILTER, PIN_MODE_OUTPUT);
    rt_pin_mode(FEED, PIN_MODE_OUTPUT);
    rt_pin_mode(FEED_DETECT_PIN, PIN_MODE_INPUT); // 语音控制输入引脚

    // 初始关闭状态
    light_on = 0;
    filter_on = 0;
    feed_on = 0;

    rt_pin_write(LIGHT, PIN_LOW);
    rt_pin_write(FILTER, PIN_LOW);
    rt_pin_write(FEED, PIN_LOW);

    // 根据语音模块输入设置 feed_on 初始状态
    int last_feed_level = rt_pin_read(FEED_DETECT_PIN);
    if (last_feed_level == PIN_HIGH)
    {
        feed_on = 1;
        rt_pin_write(FEED, PIN_HIGH);
    }

    rt_kprintf("[touch_ui] thread started\n");

    update_ui();

    rt_bool_t last_pressed = RT_FALSE;

    while (1)
    {
        int current_feed_level = rt_pin_read(FEED_DETECT_PIN);
        if (current_feed_level != last_feed_level)
        {
            last_feed_level = current_feed_level;
            feed_on = (current_feed_level == PIN_HIGH);
            rt_pin_write(FEED, feed_on ? PIN_HIGH : PIN_LOW);
            update_ui();
        }

        rt_bool_t pressed = (rt_pin_read(SPI_IRQ_PIN) == PIN_LOW);
        if (pressed && !last_pressed)
        {
            if (xpt2046_get_touch(&x, &y))
            {
                // 校准值
                int x_raw_min = 190, x_raw_max = 3853;
                int y_raw_min = 350, y_raw_max = 3979;

                x = (x < x_raw_min) ? x_raw_min : (x > x_raw_max) ? x_raw_max : x;
                y = (y < y_raw_min) ? y_raw_min : (y > y_raw_max) ? y_raw_max : y;

                int swapped_x = y, swapped_y = x;

                uint16_t px = 320 - (uint32_t)(swapped_x - y_raw_min) * 320 / (y_raw_max - y_raw_min);
                uint16_t py = 240 - (uint32_t)(swapped_y - x_raw_min) * 240 / (x_raw_max - x_raw_min);

                if (px > 319) px = 319;
                if (py > 239) py = 239;

                int id = (py - base_y) / line_height;
                if (px > ui_base_x + 8 * 8 && px < 320 && id >= 0 && id < 4)
                {
                    switch (id)
                    {
                    case 0: // Filter
                        filter_on = !filter_on;
                        rt_pin_write(FILTER, filter_on ? PIN_HIGH : PIN_LOW);
                        break;
                    case 1: // Light
                        light_on = !light_on;
                        rt_pin_write(LIGHT, light_on ? PIN_HIGH : PIN_LOW);
                        break;
                    case 2: // Heater
                        heater_on = !heater_on;
                        break;
                    case 3: // Feed
                        feed_on = !feed_on;
                        rt_pin_write(FEED, feed_on ? PIN_HIGH : PIN_LOW);
                        break;
                    default:
                        break;
                    }

                    update_ui();
                }
            }
        }

        last_pressed = pressed;
        rt_thread_mdelay(50);
    }
}
*/

static void touch_ui_thread(void *parameter)
{
    uint16_t x, y;
    soft_spi_touch_init();

    // GPIO初始化
    rt_pin_mode(LIGHT, PIN_MODE_OUTPUT);
    rt_pin_mode(FILTER, PIN_MODE_OUTPUT);
    rt_pin_mode(FEED, PIN_MODE_OUTPUT);

    rt_pin_mode(LIGHT_DETECT_PIN, PIN_MODE_INPUT);
    rt_pin_mode(FILTER_DETECT_PIN, PIN_MODE_INPUT);
    rt_pin_mode(FEED_DETECT_PIN, PIN_MODE_INPUT);

    // 默认关闭所有继电器
    rt_pin_write(LIGHT, PIN_LOW);
    rt_pin_write(FILTER, PIN_LOW);
    rt_pin_write(FEED, PIN_LOW);

    // 根据当前电平初始化状态
    light_on = (rt_pin_read(LIGHT_DETECT_PIN) == PIN_HIGH);
    filter_on = (rt_pin_read(FILTER_DETECT_PIN) == PIN_HIGH);
    feed_on = (rt_pin_read(FEED_DETECT_PIN) == PIN_HIGH);

    rt_pin_write(LIGHT, light_on ? PIN_HIGH : PIN_LOW);
    rt_pin_write(FILTER, filter_on ? PIN_HIGH : PIN_LOW);
    rt_pin_write(FEED, feed_on ? PIN_HIGH : PIN_LOW);

    // 上次状态记录
    int last_light_level = light_on;
    int last_filter_level = filter_on;
    int last_feed_level = feed_on;

    rt_kprintf("[touch_ui] thread started\n");

    update_ui();

    rt_bool_t last_pressed = RT_FALSE;

    while (1)
    {
        // === 检测语音模块电平变化 ===
        int light_level = rt_pin_read(LIGHT_DETECT_PIN);
        if (light_level != last_light_level)
        {
            light_on = (light_level == PIN_HIGH);
            rt_pin_write(LIGHT, light_on ? PIN_HIGH : PIN_LOW);
            update_ui();
            last_light_level = light_level;
        }

        int filter_level = rt_pin_read(FILTER_DETECT_PIN);
        if (filter_level != last_filter_level)
        {
            filter_on = (filter_level == PIN_HIGH);
            rt_pin_write(FILTER, filter_on ? PIN_HIGH : PIN_LOW);
            update_ui();
            last_filter_level = filter_level;
        }

        int feed_level = rt_pin_read(FEED_DETECT_PIN);
        if (feed_level != last_feed_level)
        {
            feed_on = (feed_level == PIN_HIGH);
            rt_pin_write(FEED, feed_on ? PIN_HIGH : PIN_LOW);
            update_ui();
            last_feed_level = feed_level;
        }

        // === 检测触摸屏点击 ===
        rt_bool_t pressed = (rt_pin_read(SPI_IRQ_PIN) == PIN_LOW);
        if (pressed && !last_pressed)
        {
            if (xpt2046_get_touch(&x, &y))
            {
                // 校准映射
                int x_raw_min = 190;
                int x_raw_max = 3853;
                int y_raw_min = 350;
                int y_raw_max = 3979;

                if (x < x_raw_min) x = x_raw_min;
                if (x > x_raw_max) x = x_raw_max;
                if (y < y_raw_min) y = y_raw_min;
                if (y > y_raw_max) y = y_raw_max;

                int swapped_x = y;
                int swapped_y = x;

                uint16_t px = 320 - (uint32_t)(swapped_x - y_raw_min) * 320 / (y_raw_max - y_raw_min);
                uint16_t py = 240 - (uint32_t)(swapped_y - x_raw_min) * 240 / (x_raw_max - x_raw_min);

                if (px > 319) px = 319;
                if (py > 239) py = 239;

                int id = (py - base_y) / line_height;
                if (px > ui_base_x + 8 * 8 && px < 320 && id >= 0 && id < 4)
                {
                    switch (id)
                    {
                    case 0:  // Filter
                        filter_on = !filter_on;
                        rt_pin_write(FILTER, filter_on ? PIN_HIGH : PIN_LOW);
                        break;
                    case 1:  // Light
                        light_on = !light_on;
                        rt_pin_write(LIGHT, light_on ? PIN_HIGH : PIN_LOW);
                        break;
                    case 2:  // Heater (无继电器控制)
                        heater_on = !heater_on;
                        break;
                    case 3:  // Feed
                        feed_on = !feed_on;
                        rt_pin_write(FEED, feed_on ? PIN_HIGH : PIN_LOW);
                        break;
                    }

                    update_ui();
                }
            }
        }

        last_pressed = pressed;
        rt_thread_mdelay(50); // 稍微延时，防抖/节流
    }
}


int start_touch_ui_thread(void)
{
    update_ui();  // 初始绘制UI状态
    rt_thread_t tid = rt_thread_create("touch_ui", touch_ui_thread, RT_NULL, 1024, 20, 10);
    if (tid)
    {
        rt_kprintf("[main] touch_ui thread created\n");
        rt_thread_startup(tid);
    }
    else
    {
        rt_kprintf("[main] touch_ui thread creation failed!\n");
    }

    return 0;
}

int main(void)
{
    spi1_init();
    ili9341_gpio_init();
    ili9341_init();
    ili9341_fill_color(WHITE);

    wifi_init();
    relay_gpio_init();

    lcd_draw_string(10, 10, "Touch Test Start", BLACK, WHITE);

    start_touch_ui_thread();



    rt_thread_t tid = rt_thread_create("lcd_disp", sensor_display_thread,
                                       RT_NULL, 1024, 15, 10);
    if (tid != RT_NULL)
    {
        rt_kprintf("[main] lcd_disp thread created.\n");
        rt_thread_startup(tid);
    }
    else
    {
        rt_kprintf("[main] lcd_disp thread creation failed!\n");
    }

    return 0;
}



