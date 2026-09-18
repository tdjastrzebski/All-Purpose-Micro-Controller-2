#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "spi_drv.h"

typedef struct {
	uint8_t R;
	uint8_t G;
	uint8_t B;
} st7789_color;

const st7789_color st7789_color_white{R : 0x1f, G : 0x3f, B : 0x1f};  // RGB565: white
const st7789_color st7789_color_black{R : 0x0, G : 0x0, B : 0x0};     // RGB565: black
const st7789_color st7789_color_red{R : 0x1f, G : 0x0, B : 0x0};      // RGB565: red
const st7789_color st7789_color_green{R : 0x0, G : 0x3f, B : 0x0};    // RGB565: green
const st7789_color st7789_color_blue{R : 0x0, G : 0x0, B : 0x1f};     // RGB565: blue

void st7789_Init(spi_channel_dev_ctx* dev_ctx);
void st7789_FillScreen(spi_channel_dev_ctx* dev_ctx, st7789_color color);
void st7789_DrawPixel(spi_channel_dev_ctx* dev_ctx, uint16_t x, uint16_t y, st7789_color color);
void st7789_SetWindow(spi_channel_dev_ctx* dev_ctx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

#ifdef __cplusplus
}
#endif