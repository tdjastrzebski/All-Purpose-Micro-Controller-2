#include "spi_drv.h"

void spi_drv_configureSpi(spi_channel_dev_ctx* dev_ctx, spi_drv_direction direction, spi_drv_mode mode) {
	dev_ctx->channel->Instance->CR1 &= ~SPI_CR1_SPE;  // disable SPI
	if (mode == spi_drv_mode_0 || mode == spi_drv_mode_1) {
		dev_ctx->channel->Instance->CFG2 &= ~SPI_CFG2_CPOL;  // clk polarity low
	} else {
		dev_ctx->channel->Instance->CFG2 |= SPI_CFG2_CPOL;  // clk polarity high
	}
	if (mode == spi_drv_mode_1 || mode == spi_drv_mode_3) {
		dev_ctx->channel->Instance->CFG2 |= SPI_CFG2_CPHA;  // data on 2nd clk edge
	} else {
		dev_ctx->channel->Instance->CFG2 &= ~SPI_CFG2_CPHA;  // data on 1nd clk edge
	}
	if (direction == spi_drv_direction_fullDuplex) {
		// full duplex
		dev_ctx->channel->Instance->CFG2 &= ~SPI_CFG2_COMM;
	} else {
		// half-duplex
		dev_ctx->channel->Instance->CFG2 |= SPI_CFG2_COMM;
	}
	dev_ctx->channel->Instance->CFG2 &= ~SPI_CFG2_LSBFRST;                                 // disable LSB first
	dev_ctx->channel->Instance->CFG2 &= ~(SPI_CFG2_SP_0 | SPI_CFG2_SP_1 | SPI_CFG2_SP_2);  // reset SPI_CFG2_SP bits - Motorola mode
	dev_ctx->channel->Instance->CR1 |= SPI_CR1_SPE;                                        // enable SPI
}