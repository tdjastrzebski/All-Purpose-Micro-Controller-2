#include "st7789.h"

#include "main.h"
#include "spi_drv.h"

#define DC_LOW() HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET)
#define DC_HIGH() HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET)
#define Y_OFFSET 20  // st7789V3 internally has 240x320 buffer, 240x280 LCD uses buffer from 20th line

// references: https://files.waveshare.com/wiki/common/ST7789VW.pdf

static void _CS_LOW(spi_channel_dev_ctx* dev_ctx) {
	HAL_GPIO_WritePin(dev_ctx->cs_port, dev_ctx->cs_pin, GPIO_PIN_RESET);
}

static void _CS_HIGH(spi_channel_dev_ctx* dev_ctx) {
	HAL_GPIO_WritePin(dev_ctx->cs_port, dev_ctx->cs_pin, GPIO_PIN_SET);
}

void st7789_WriteCommand(spi_channel_dev_ctx* dev_ctx, uint8_t cmd) {
	DC_LOW();
	_CS_LOW(dev_ctx);
	HAL_SPI_Transmit(dev_ctx->channel, &cmd, 1, HAL_MAX_DELAY);
	_CS_HIGH(dev_ctx);
}

void st7789_WriteData(spi_channel_dev_ctx* dev_ctx, uint8_t* data, uint16_t size) {
	DC_HIGH();
	_CS_LOW(dev_ctx);
	HAL_SPI_Transmit(dev_ctx->channel, data, size, HAL_MAX_DELAY);
	_CS_HIGH(dev_ctx);
}

void st7789_WriteData(spi_channel_dev_ctx* dev_ctx, uint8_t data) {
	DC_HIGH();
	_CS_LOW(dev_ctx);
	HAL_SPI_Transmit(dev_ctx->channel, &data, 1, HAL_MAX_DELAY);
	_CS_HIGH(dev_ctx);
}

void st7789_Init(spi_channel_dev_ctx* dev_ctx) {
	// Hardware reset
	// RST_LOW();
	HAL_Delay(10);
	// RST_HIGH();
	HAL_Delay(120);

	st7789_WriteCommand(dev_ctx, 0x01);  // Software reset (SWRESET)
	HAL_Delay(150);

	st7789_WriteCommand(dev_ctx, 0x11);  // Sleep out (SLPOUT)
	HAL_Delay(500);

	// 8.8 Data Color Coding
	// 8.8.36 3-Line Serial Interface
	// 8.8.40 4-Line Serial Interface
	// Pixel format: 16-bit (12b 0x03, 16b 0x05, 18b 0x06)
	st7789_WriteCommand(dev_ctx, 0x3A);
	st7789_WriteData(dev_ctx, 0x55);

	// 9.1.28 MADCTL (36h): Memory Data Access Control
	st7789_WriteCommand(dev_ctx, 0x36);
	st7789_WriteData(dev_ctx, 0x00);

	// INVON - invert color bits
	st7789_WriteCommand(dev_ctx, 0x21);
	st7789_WriteData(dev_ctx, 0x21);

	st7789_WriteCommand(dev_ctx, 0x29);  // Display on (DISPON) - see: 9.1 System Function Command Table 1
	HAL_Delay(100);
}

void st7789_SetWindow(spi_channel_dev_ctx* dev_ctx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
	uint8_t data[4];
	y0 += Y_OFFSET;
	y1 += Y_OFFSET;

	st7789_WriteCommand(dev_ctx, 0x2A);  // Column addr (CASET)
	data[0] = x0 >> 8;
	data[1] = x0 & 0xFF;
	data[2] = x1 >> 8;
	data[3] = x1 & 0xFF;
	st7789_WriteData(dev_ctx, data, 4);

	st7789_WriteCommand(dev_ctx, 0x2B);  // Row addr (RASET)
	data[0] = y0 >> 8;
	data[1] = y0 & 0xFF;
	data[2] = y1 >> 8;
	data[3] = y1 & 0xFF;
	st7789_WriteData(dev_ctx, data, 4);

	st7789_WriteCommand(dev_ctx, 0x2C);  // Memory write (RAMWR)
}

void st7789_FillScreen(spi_channel_dev_ctx* dev_ctx, st7789_color color) {
	st7789_SetWindow(dev_ctx, 0, 0, 239, 279);
	uint8_t data[2];
	data[0] = (color.R << 3) | ((color.G & 0x38) >> 3);
	data[1] = (color.G << 5) | (color.B & 0x1f);
	DC_HIGH();
	_CS_LOW(dev_ctx);
	for (uint32_t i = 0; i < 240UL * 280UL; i++) {
		HAL_SPI_Transmit(dev_ctx->channel, data, 2, HAL_MAX_DELAY);
	}
	_CS_HIGH(dev_ctx);
}

void st7789_DrawPixel(spi_channel_dev_ctx* dev_ctx, uint16_t x, uint16_t y, st7789_color color) {
	st7789_SetWindow(dev_ctx, x, y, x, y);
	uint8_t data[2];
	data[0] = (color.R << 3) | ((color.G & 0x38) >> 3);
	data[1] = (color.G << 5) | (color.B & 0x1f);
	DC_HIGH();
	_CS_LOW(dev_ctx);
	HAL_SPI_Transmit(dev_ctx->channel, data, 2, HAL_MAX_DELAY);
	_CS_HIGH(dev_ctx);
}