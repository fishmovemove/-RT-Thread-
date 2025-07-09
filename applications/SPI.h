/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-29     fish move move       the first version
 */
#ifndef __SOFT_SPI_TOUCH_H__
#define __SOFT_SPI_TOUCH_H__

#include <rtthread.h>
#include <rtdevice.h>
#include <stdint.h>

void soft_spi_touch_init(void);
rt_bool_t xpt2046_get_touch(uint16_t *x, uint16_t *y);


#endif

