// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <driver/gpio.h>
#include <components/media_types.h>
#include <driver/lcd_types.h>
#include <driver/sim_spi.h>
#include "bk_misc.h"
#include "lcd_panel_devices.h"
#include "gpio_driver.h"
#include <driver/lcd.h>



static const lcd_rgb_t lcd_rgb =
{
	.clk = LCD_20M,
	.data_out_clk_edge = NEGEDGE_OUTPUT,

	.hsync_pulse_width = 20,
	.vsync_pulse_width = 4,
	.hsync_back_porch = 60,
	.hsync_front_porch = 40,
	.vsync_back_porch = 19,
	.vsync_front_porch = 12,
};

static void lcd_st7701s_config(void)
{
	#define Delay rtos_delay_milliseconds
	#define SPI_WriteComm lcd_spi_write_cmd
	#define SPI_WriteData lcd_spi_write_data

	rtos_delay_milliseconds(120);
	bk_gpio_set_output_low(LCD_SPI_RST);
	rtos_delay_milliseconds(120);
	bk_gpio_set_output_high(LCD_SPI_RST);
	rtos_delay_milliseconds(120);

	SPI_WriteComm (0xFF);
	SPI_WriteData (0x77);
	SPI_WriteData (0x01);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x13);
	SPI_WriteComm (0xEF);
	SPI_WriteData (0x08);
	SPI_WriteComm (0xFF);
	SPI_WriteData (0x77);
	SPI_WriteData (0x01);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x10);
	SPI_WriteComm (0xC0);
	SPI_WriteData (0x3B);
	SPI_WriteData (0x00);
	SPI_WriteComm (0xC1);
	SPI_WriteData (0x0D);
	SPI_WriteData (0x02);
	SPI_WriteComm (0xC2);
	SPI_WriteData (0x21);
	SPI_WriteData (0x08);
	SPI_WriteComm (0xC7);
	SPI_WriteData (0x04);
	SPI_WriteComm (0xCC);
	SPI_WriteData (0x18);
	SPI_WriteComm (0xB0);
	SPI_WriteData (0x00);
	SPI_WriteData (0x11);
	SPI_WriteData (0x17);
	SPI_WriteData (0x0E);
	SPI_WriteData (0x12);
	SPI_WriteData (0x06);
	SPI_WriteData (0x06);
	SPI_WriteData (0x08);
	SPI_WriteData (0x08);
	SPI_WriteData (0x20);
	SPI_WriteData (0x04);
	SPI_WriteData (0x11);
	SPI_WriteData (0x0F);
	SPI_WriteData (0x29);
	SPI_WriteData (0x30);
	SPI_WriteData (0x1F);
	SPI_WriteComm (0xB1);
	SPI_WriteData (0x00);
	SPI_WriteData (0x13);
	SPI_WriteData (0x18);
	SPI_WriteData (0x0F);
	SPI_WriteData (0x12);
	SPI_WriteData (0x07);
	SPI_WriteData (0x06);
	SPI_WriteData (0x08);
	SPI_WriteData (0x07);
	SPI_WriteData (0x21);
	SPI_WriteData (0x04);
	SPI_WriteData (0x12);
	SPI_WriteData (0x10);
	SPI_WriteData (0x29);
	SPI_WriteData (0x34);
	SPI_WriteData (0x1F);
	SPI_WriteComm (0xFF);
	SPI_WriteData (0x77);
	SPI_WriteData (0x01);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x11);
	SPI_WriteComm (0xB0);
	SPI_WriteData (0x60);
	SPI_WriteComm (0xB1);
	SPI_WriteData (0x3A);
	SPI_WriteComm (0xB2);
	SPI_WriteData (0x8A);
	SPI_WriteComm (0xB3);
	SPI_WriteData (0x80);
	SPI_WriteComm (0xB5);
	SPI_WriteData (0x4B);
	SPI_WriteComm (0xB7);
	SPI_WriteData (0x85);
	SPI_WriteComm (0xB8);
	SPI_WriteData (0x21);
	SPI_WriteComm (0xC0);
	SPI_WriteData (0x07);
	SPI_WriteComm (0xC1);
	SPI_WriteData (0x78);
	SPI_WriteComm (0xC2);
	SPI_WriteData (0x78);
	SPI_WriteComm (0xE0);
	SPI_WriteData (0x00);
	SPI_WriteData (0x1B);
	SPI_WriteData (0x02);
	SPI_WriteComm (0xE1);
	SPI_WriteData (0x08);
	SPI_WriteData (0xA0);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x07);
	SPI_WriteData (0xA0);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x44);
	SPI_WriteData (0x44);
	SPI_WriteComm (0xE2);
	SPI_WriteData (0x11);
	SPI_WriteData (0x11);
	SPI_WriteData (0x44);
	SPI_WriteData (0x44);
	SPI_WriteData (0xED);
	SPI_WriteData (0xA0);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0xEC);
	SPI_WriteData (0xA0);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteComm (0xE3);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x11);
	SPI_WriteData (0x11);
	SPI_WriteComm (0xE4);
	SPI_WriteData (0x44);
	SPI_WriteData (0x44);
	SPI_WriteComm (0xE5);
	SPI_WriteData (0x0A);
	SPI_WriteData (0xE9);
	SPI_WriteData (0xD8);
	SPI_WriteData (0xA0);
	SPI_WriteData (0x0C);
	SPI_WriteData (0xEB);
	SPI_WriteData (0xD8);
	SPI_WriteData (0xA0);
	SPI_WriteData (0x0E);
	SPI_WriteData (0xED);
	SPI_WriteData (0xD8);
	SPI_WriteData (0xA0);
	SPI_WriteData (0x10);
	SPI_WriteData (0xEF);
	SPI_WriteData (0xD8);
	SPI_WriteData (0xA0);
	SPI_WriteComm (0xE6);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x11);
	SPI_WriteData (0x11);
	SPI_WriteComm (0xE7);
	SPI_WriteData (0x44);
	SPI_WriteData (0x44);
	SPI_WriteComm (0xE8);
	SPI_WriteData (0x09);
	SPI_WriteData (0xE8);
	SPI_WriteData (0xD8);
	SPI_WriteData (0xA0);
	SPI_WriteData (0x0B);
	SPI_WriteData (0xEA);
	SPI_WriteData (0xD8);
	SPI_WriteData (0xA0);
	SPI_WriteData (0x0D);
	SPI_WriteData (0xEC);
	SPI_WriteData (0xD8);
	SPI_WriteData (0xA0);
	SPI_WriteData (0x0F);
	SPI_WriteData (0xEE);
	SPI_WriteData (0xD8);
	SPI_WriteData (0xA0);
	SPI_WriteComm (0xEB);
	SPI_WriteData (0x02);
	SPI_WriteData (0x00);
	SPI_WriteData (0xE4);
	SPI_WriteData (0xE4);
	SPI_WriteData (0x88);
	SPI_WriteData (0x00);
	SPI_WriteData (0x40);
	SPI_WriteComm (0xEC);
	SPI_WriteData (0x3C);
	SPI_WriteData (0x00);
	SPI_WriteComm (0xED);
	SPI_WriteData (0xAB);
	SPI_WriteData (0x89);
	SPI_WriteData (0x76);
	SPI_WriteData (0x54);
	SPI_WriteData (0x02);
	SPI_WriteData (0xFF);
	SPI_WriteData (0xFF);
	SPI_WriteData (0xFF);
	SPI_WriteData (0xFF);
	SPI_WriteData (0xFF);
	SPI_WriteData (0xFF);
	SPI_WriteData (0x20);
	SPI_WriteData (0x45);
	SPI_WriteData (0x67);
	SPI_WriteData (0x98);
	SPI_WriteData (0xBA);
	SPI_WriteComm (0xEF);
	SPI_WriteData (0x08);
	SPI_WriteData (0x08);
	SPI_WriteData (0x08);
	SPI_WriteData (0x45);
	SPI_WriteData (0x3F);
	SPI_WriteData (0x54);
	SPI_WriteComm (0xFF);
	SPI_WriteData (0x77);
	SPI_WriteData (0x01);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x13);
	SPI_WriteComm (0xE8);
	SPI_WriteData (0x00);
	SPI_WriteData (0x0E);
	SPI_WriteComm (0xFF);
	SPI_WriteData (0x77);
	SPI_WriteData (0x01);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteComm (0x11);
	Delay(120);
	SPI_WriteComm (0xFF);
	SPI_WriteData (0x77);
	SPI_WriteData (0x01);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x13);
	SPI_WriteComm (0xE8);
	SPI_WriteData (0x00);
	SPI_WriteData (0x0C);
	Delay(10);
	SPI_WriteComm (0xE8);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteComm (0xFF);
	SPI_WriteData (0x77);
	SPI_WriteData (0x01);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);
	SPI_WriteData (0x00);

	SPI_WriteComm(0x3A);
	SPI_WriteData(0x55);

	SPI_WriteComm(0x36);
	SPI_WriteData(0x10);

	SPI_WriteComm(0x11);
	Delay(150);
// SPI_WriteComm(0x21);
	SPI_WriteComm(0x29);
	Delay(20);
}

static bk_err_t lcd_st7701s_init(const void *handle)
{
#if 1
	BK_LOGD(NULL, "lcd_st7701s: init.\r\n");
#endif
	lcd_spi_init_gpio();
	lcd_st7701s_config();
	return BK_OK;
}

const lcd_device_t lcd_device_st7701s_app =
{
	.id = LCD_DEVICE_ST7701S,
	.name = "st7701s",
	.type = LCD_TYPE_RGB565,
	.src_fmt = PIXEL_FMT_RGB565,
	.out_fmt = PIXEL_FMT_RGB565,
	.width = 480,
	.height = 480,
	.rgb = &lcd_rgb,
	.init = lcd_st7701s_init,
	.off = NULL,
};
